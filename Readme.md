# Submission Format

Each team submits files only inside `teams/<team>/`.

Required contract for the solution:
- `make build` must produce an executable named `solution`
- `./solution <path-to-input-file>` must read input from that file
- the program must write the answer to `stdout`

The runner checks tests through:

```bash
make test INPUT_FILE=path/to/input.in OUTPUT_FILE=path/to/output.ans
```

So the `test` target in `Makefile` should run `./solution "$INPUT_FILE"` and redirect stdout to `OUTPUT_FILE`.
