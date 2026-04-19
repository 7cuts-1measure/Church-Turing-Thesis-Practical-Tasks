#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def load_config(task_dir: Path) -> dict:
    config_path = task_dir / "task.json"
    if not config_path.exists():
        raise SystemExit(f"Missing task config: {config_path}")
    return json.loads(config_path.read_text(encoding="utf-8"))


def resolve_task_roots(
    task_dir: Path,
    candidate_root: Path,
    baseline_root: Path | None,
) -> tuple[Path, Path | None, Path]:
    if baseline_root is not None:
        try:
            task_rel = task_dir.relative_to(baseline_root)
        except ValueError as exc:
            raise SystemExit(
                f"Task directory {task_dir} is not inside baseline root {baseline_root}"
            ) from exc
    else:
        task_rel = Path(task_dir.name)

    if str(task_rel) == ".":
        candidate_task_root = candidate_root
        baseline_task_root = baseline_root if baseline_root is not None else None
    else:
        candidate_task_root = candidate_root / task_rel
        baseline_task_root = baseline_root / task_rel if baseline_root is not None else None
    return candidate_task_root, baseline_task_root, task_rel


def collect_changed_files(candidate_root: Path, baseline_root: Path | None) -> list[Path]:
    changed: list[Path] = []
    for path in sorted(p for p in candidate_root.rglob("*") if p.is_file()):
        rel = path.relative_to(candidate_root)
        if rel.parts and rel.parts[0] == ".git":
            continue
        if baseline_root is None:
            changed.append(rel)
            continue

        baseline_path = baseline_root / rel
        if not baseline_path.exists() or path.read_bytes() != baseline_path.read_bytes():
            changed.append(rel)

    if baseline_root is not None:
        for path in sorted(p for p in baseline_root.rglob("*") if p.is_file()):
            rel = path.relative_to(baseline_root)
            if rel.parts and rel.parts[0] == ".git":
                continue
            candidate_path = candidate_root / rel
            if not candidate_path.exists():
                changed.append(rel)

    return sorted(set(changed))


def validate_change_scope(changed_files: list[Path], submission_root: str, task_rel: Path) -> str:
    changed_teams: set[str] = set()

    for rel_path in changed_files:
        parts = rel_path.parts
        if len(parts) < 2 or parts[0] != submission_root:
            continue
        changed_teams.add(parts[1])

    if not changed_teams:
        raise SystemExit(f"No changed files found under submission root: {submission_root}")
    if len(changed_teams) != 1:
        teams = ", ".join(sorted(changed_teams))
        raise SystemExit(f"Changes affect multiple team directories: {teams}")

    return next(iter(changed_teams))


def validate_team_dir(team_dir: Path) -> None:
    if not team_dir.exists():
        raise SystemExit(f"Missing team directory: {team_dir}")
    if not team_dir.is_dir():
        raise SystemExit(f"Team path is not a directory: {team_dir}")

    makefile_path = team_dir / "Makefile"
    if not makefile_path.exists():
        raise SystemExit(f"Missing Makefile in team directory: {makefile_path}")
    if not makefile_path.is_file():
        raise SystemExit(f"Makefile path is not a regular file: {makefile_path}")

    team_hash_path = team_dir / "team.hash"
    if not team_hash_path.exists():
        raise SystemExit(f"Missing team.hash in team directory: {team_hash_path}")
    if not team_hash_path.is_file():
        raise SystemExit(f"team.hash path is not a regular file: {team_hash_path}")
    if not team_hash_path.read_text(encoding="utf-8").strip():
        raise SystemExit(f"team.hash is empty in team directory: {team_hash_path}")


def validate_team_name(team_name: str) -> None:
    reserved_names = {"<team>", "example"}
    if team_name in reserved_names:
        names = ", ".join(sorted(reserved_names))
        raise SystemExit(
            f"Reserved team directory '{team_name}' cannot be submitted. "
            f"Choose a real team directory instead of one of: {names}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--task-dir", required=True)
    parser.add_argument("--candidate-root", required=True)
    parser.add_argument("--baseline-root")
    args = parser.parse_args()

    task_dir = Path(args.task_dir).resolve()
    candidate_root = Path(args.candidate_root).resolve()
    baseline_root = Path(args.baseline_root).resolve() if args.baseline_root else None

    if not task_dir.exists():
        raise SystemExit(f"Missing task directory: {task_dir}")
    if not candidate_root.exists():
        raise SystemExit(f"Missing candidate root: {candidate_root}")
    if baseline_root is not None and not baseline_root.exists():
        raise SystemExit(f"Missing baseline root: {baseline_root}")

    config = load_config(task_dir)
    submission_root = config.get("submission_root", "")
    if not submission_root:
        raise SystemExit("Missing 'submission_root' in task config")

    candidate_task_root, baseline_task_root, task_rel = resolve_task_roots(
        task_dir,
        candidate_root,
        baseline_root,
    )
    if not candidate_task_root.exists():
        raise SystemExit(f"Missing task directory in candidate tree: {candidate_task_root}")
    if baseline_task_root is not None and not baseline_task_root.exists():
        raise SystemExit(f"Missing task directory in baseline tree: {baseline_task_root}")

    changed_files = collect_changed_files(candidate_task_root, baseline_task_root)
    detected_team = validate_change_scope(changed_files, submission_root, task_rel)
    validate_team_name(detected_team)

    team_dir = candidate_task_root / submission_root / detected_team
    validate_team_dir(team_dir)

    print(team_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
