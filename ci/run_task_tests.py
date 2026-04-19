#!/usr/bin/env python3
import argparse
import json
import os
import resource
import shutil
import subprocess
import tempfile
import time
from pathlib import Path

MAX_LOG_CHARS = 2000
DEFAULT_MEMORY_LIMIT_MB = 512


def load_config(task_dir: Path) -> dict:
    config_path = task_dir / "task.json"
    if not config_path.exists():
        raise SystemExit(f"Missing task config: {config_path}")
    return json.loads(config_path.read_text())


def resolve_scoring_group(config: dict, visibility: str) -> tuple[str, int]:
    scoring = config.get("scoring", {})
    group = scoring.get(visibility, {})
    label = group.get("label", visibility.capitalize())
    weight = group.get("weight_per_test", 0)
    if not isinstance(weight, int):
        raise SystemExit(
            f"Invalid weight_per_test for scoring group '{visibility}': expected integer"
        )
    return label, weight


def trim_log(text: str) -> str:
    if len(text) <= MAX_LOG_CHARS:
        return text
    return text[:MAX_LOG_CHARS] + "...<truncated>"


def detect_sandbox_backend(requested: str) -> str:
    if requested == "auto":
        if shutil.which("bwrap"):
            return "bwrap"
        return "none"

    if requested != "none" and shutil.which(requested) is None:
        raise SystemExit(f"Requested sandbox backend is not available: {requested}")

    return requested


def minimal_env() -> dict[str, str]:
    path = os.environ.get("PATH")
    if not path:
        path = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
    return {
        "PATH": path,
        "LANG": "C.UTF-8",
        "LC_ALL": "C.UTF-8",
    }


def resolve_make_command() -> str:
    make_path = shutil.which("make")
    if make_path is None:
        raise SystemExit(
            "Required build tool 'make' is not available in PATH on the runner"
        )
    return make_path


def limit_process_resources(timeout_sec: int, memory_limit_mb: int):
    def apply_limits() -> None:
        cpu_limit = max(timeout_sec + 1, 1)
        resource.setrlimit(resource.RLIMIT_CPU, (cpu_limit, cpu_limit))
        if memory_limit_mb > 0:
            memory_limit_bytes = memory_limit_mb * 1024 * 1024
            resource.setrlimit(resource.RLIMIT_AS, (memory_limit_bytes, memory_limit_bytes))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

    return apply_limits


def build_command(
    submission_dir: Path,
    sandbox_backend: str,
    inner_command: list[str],
) -> list[str]:
    if sandbox_backend == "none":
        return inner_command

    if sandbox_backend == "bwrap":
        cmd = [
            "bwrap",
            "--die-with-parent",
            "--new-session",
            "--unshare-user-try",
            "--unshare-pid",
            "--unshare-ipc",
            "--unshare-uts",
            "--share-net",
            "--proc",
            "/proc",
            "--dev",
            "/dev",
            "--tmpfs",
            "/tmp",
            "--bind",
            str(submission_dir),
            "/workspace",
            "--chdir",
            "/workspace",
        ]

        for system_path in ("/usr", "/bin", "/lib", "/lib64", "/etc"):
            if Path(system_path).exists():
                cmd.extend(["--ro-bind", system_path, system_path])

        cmd.extend(inner_command)
        return cmd

    raise SystemExit(f"Unsupported sandbox backend: {sandbox_backend}")


def build_bwrap_test_command(
    sandbox_root: Path,
    sandbox_workspace: Path,
    inner_command: list[str],
) -> list[str]:
    cmd = [
        "bwrap",
        "--die-with-parent",
        "--new-session",
        "--unshare-user-try",
        "--unshare-pid",
        "--unshare-ipc",
        "--unshare-uts",
        "--share-net",
        "--proc",
        "/proc",
        "--dev",
        "/dev",
        "--tmpfs",
        "/tmp",
        "--bind",
        str(sandbox_root),
        "/sandbox",
        "--bind",
        str(sandbox_workspace),
        "/workspace",
        "--chdir",
        "/workspace",
    ]

    for system_path in ("/usr", "/bin", "/lib", "/lib64", "/etc"):
        if Path(system_path).exists():
            cmd.extend(["--ro-bind", system_path, system_path])

    cmd.extend(inner_command)
    return cmd


def run_command(
    command: list[str],
    cwd: Path,
    timeout_sec: int,
    memory_limit_mb: int,
) -> tuple[subprocess.CompletedProcess[str], float]:
    try:
        started = time.perf_counter()
        result = subprocess.run(
            command,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=timeout_sec,
            env=minimal_env(),
            preexec_fn=limit_process_resources(timeout_sec, memory_limit_mb),
        )
        elapsed = time.perf_counter() - started
    except subprocess.TimeoutExpired:
        raise RuntimeError(f"Timed out after {timeout_sec}s") from None

    return result, elapsed


def ensure_success(result: subprocess.CompletedProcess[str], context: str) -> None:
    if result.returncode == 0:
        return

    stderr = trim_log(result.stderr.strip())
    stdout = trim_log(result.stdout.strip())
    details = []
    if stdout:
        details.append(f"stdout: {stdout}")
    if stderr:
        details.append(f"stderr: {stderr}")
    joined = " | ".join(details)
    if joined:
        raise RuntimeError(f"{context} failed with exit code {result.returncode}: {joined}")
    raise RuntimeError(f"{context} failed with exit code {result.returncode}")


def run_make_build(
    submission_dir: Path,
    make_command: str,
    sandbox_backend: str,
    timeout_sec: int,
    memory_limit_mb: int,
) -> None:
    inner_command = [make_command, "build"]
    command = build_command(submission_dir, sandbox_backend, inner_command)
    result, _ = run_command(command, submission_dir, timeout_sec, memory_limit_mb)
    ensure_success(result, "make build")


def resolve_build_sandbox_backend(sandbox_backend: str) -> str:
    # Building untrusted code is much less sensitive than executing it, and
    # compilers/toolchains often need broader host filesystem access than bwrap
    # permits on self-hosted runners. Keep sandboxing for test execution.
    if sandbox_backend == "bwrap":
        return "none"
    return sandbox_backend


def run_make_test(
    submission_dir: Path,
    make_command: str,
    test_file: Path,
    tests_root: Path,
    sandbox_backend: str,
    timeout_sec: int,
    memory_limit_mb: int,
) -> float:
    with tempfile.TemporaryDirectory(prefix="task-test-out-") as temp_dir_name:
        temp_dir = Path(temp_dir_name)

        if sandbox_backend == "bwrap":
            sandbox_root = temp_dir / "sandbox"
            sandbox_workspace = sandbox_root / "workspace"
            sandbox_tests_root = sandbox_root / "tests"
            sandbox_test = sandbox_tests_root / test_file.relative_to(tests_root)

            shutil.copytree(submission_dir, sandbox_workspace)
            shutil.copytree(tests_root, sandbox_tests_root)

            inner_command = [
                make_command,
                "test",
                f"TEST_FILE=/sandbox/tests/{test_file.relative_to(tests_root)}",
            ]
            command = build_bwrap_test_command(sandbox_root, sandbox_workspace, inner_command)
            result, elapsed = run_command(command, sandbox_workspace, timeout_sec, memory_limit_mb)
            ensure_success(result, f"make test for {test_file.name}")
        else:
            inner_command = [
                make_command,
                "test",
                f"TEST_FILE={test_file}",
            ]
            command = build_command(submission_dir, sandbox_backend, inner_command)
            result, elapsed = run_command(command, submission_dir, timeout_sec, memory_limit_mb)
            ensure_success(result, f"make test for {test_file.name}")

        return elapsed


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--task-dir", required=True)
    parser.add_argument("--submission-dir", required=True)
    parser.add_argument("--tests-dir", required=True)
    parser.add_argument("--visibility", choices=["public", "hidden"], default="public")
    parser.add_argument("--json-output")
    parser.add_argument("--timeout-sec", type=int, default=5)
    parser.add_argument("--build-timeout-sec", type=int, default=30)
    parser.add_argument("--sandbox-backend", choices=["auto", "none", "bwrap"], default="auto")
    parser.add_argument("--require-sandbox", action="store_true")
    parser.add_argument("--memory-limit-mb", type=int, default=DEFAULT_MEMORY_LIMIT_MB)
    args = parser.parse_args()

    task_dir = Path(args.task_dir).resolve()
    config = load_config(task_dir)

    submission_dir = Path(args.submission_dir).resolve()
    tests_dir = Path(args.tests_dir).resolve()

    if not submission_dir.exists():
        raise SystemExit(f"Missing submission directory: {submission_dir}")
    if not (submission_dir / "Makefile").exists():
        raise SystemExit(f"Missing Makefile in submission directory: {submission_dir}")
    if not tests_dir.exists():
        raise SystemExit(f"Missing tests directory: {tests_dir}")

    group_name, test_weight = resolve_scoring_group(config, args.visibility)
    json_output = Path(args.json_output).resolve() if args.json_output else None
    make_command = resolve_make_command()
    sandbox_backend = detect_sandbox_backend(args.sandbox_backend)
    if args.require_sandbox and sandbox_backend == "none":
        raise SystemExit("Sandbox is required, but no supported sandbox backend is available")
    build_sandbox_backend = resolve_build_sandbox_backend(sandbox_backend)

    print(f"Execution sandbox: {sandbox_backend}")
    if build_sandbox_backend != sandbox_backend:
        print(f"Build sandbox: {build_sandbox_backend}")
    print(f"Building submission in {submission_dir}...")
    run_make_build(
        submission_dir,
        make_command,
        build_sandbox_backend,
        args.build_timeout_sec,
        args.memory_limit_mb,
    )

    test_files = sorted(tests_dir.glob("*.c"))
    if not test_files:
        raise SystemExit(f"No C tests found in {tests_dir}")
    tests_root = tests_dir.parent

    is_public = args.visibility == "public"
    passed = 0
    failed = 0
    total_time = 0.0
    passed_tests: list[str] = []
    failed_tests: list[str] = []

    for test_file in test_files:
        try:
            elapsed = run_make_test(
                submission_dir,
                make_command,
                test_file,
                tests_root,
                sandbox_backend,
                args.timeout_sec,
                args.memory_limit_mb,
            )
            total_time += elapsed
            passed += 1
            if is_public:
                passed_tests.append(test_file.stem)
                print(f"[PASS] {test_file.stem} ({elapsed:.3f}s)")
        except Exception as exc:
            failed += 1
            if is_public:
                failed_tests.append(test_file.stem)
                print(f"[FAIL] {test_file.stem}")
                print(str(exc))

    total_tests = len(test_files)
    points_total = total_tests * test_weight
    points_awarded = passed * test_weight

    print(f"Passed {passed}/{total_tests} tests in {total_time:.3f}s")
    print(f"Failed: {failed}")
    if test_weight:
        print(f"Points: {points_awarded}/{points_total}")
    if is_public and passed_tests:
        print("Passed tests: " + ", ".join(passed_tests))
    if is_public and failed_tests:
        print("Failed tests: " + ", ".join(failed_tests))

    if json_output:
        summary = {
            "task": config.get("name", task_dir.name),
            "group_name": group_name,
            "visibility": args.visibility,
            "test_weight": test_weight,
            "total_tests": total_tests,
            "passed_tests": passed,
            "failed_tests": failed,
            "points_awarded": points_awarded,
            "points_total": points_total,
            "passed_test_names": passed_tests if is_public else [],
            "failed_test_names": failed_tests if is_public else [],
            "total_time_sec": round(total_time, 3),
            "success": failed == 0,
        }
        json_output.write_text(
            json.dumps(summary, ensure_ascii=True, indent=2) + "\n",
            encoding="utf-8",
        )

    if failed:
        raise SystemExit(1)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
