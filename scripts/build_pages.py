"""Generate browser data from the actual C++ simulator CSV output."""
import csv
import json
import pathlib
import subprocess
import tempfile
import sys

executable = sys.argv[1] if len(sys.argv) > 1 else "./build/life_tree"
destination = pathlib.Path("docs/data.js")
with tempfile.TemporaryDirectory() as temporary:
    subprocess.run([executable, "--strategy", "all", "--days", "1095", "--output", temporary], check=True)
    data = {}
    for strategy in ("money", "health", "balanced"):
        with open(pathlib.Path(temporary) / f"{strategy}.csv") as source:
            data[strategy] = [{key: float(value) for key, value in row.items()} for row in csv.DictReader(source)]
    columns = list(data["money"][0])
    compact = {strategy: [[row[key] for key in columns] for row in rows] for strategy, rows in data.items()}
    payload = json.dumps(compact, separators=(",", ":"))
    destination.write_text(
        "const lifeColumns = " + json.dumps(columns) + ";\n"
        "window.LIFE_DATA = Object.fromEntries(Object.entries(" + payload + ").map(([strategy, rows]) => "
        "[strategy, rows.map(row => Object.fromEntries(lifeColumns.map((key, index) => [key, row[index]])))]));\n"
    )
print(f"Generated {destination}")
