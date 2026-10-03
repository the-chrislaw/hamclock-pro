"""Exercise the executable's public CLI in a disposable local directory."""
import json
import pathlib
import stat
import subprocess
import sys
import tempfile

binary = str(pathlib.Path(sys.argv[1]).resolve())


def run(*args, code=0):
    result = subprocess.run([binary, *args], capture_output=True, text=True, timeout=10)
    assert result.returncode == code, (args, result.returncode, result.stderr)
    return result


east = json.loads(run("--calculate", "0", "0", "0", "1").stdout)
assert abs(east["distance_km"] - 111.195080) < 0.000001
assert east["initial_bearing_deg"] == 90
assert east["de_grid"] == "JJ00aa"
for coordinates in [("nan", "0", "0", "0"), ("91", "0", "0", "0")]:
    assert run("--calculate", *coordinates, code=2).stdout == ""
assert run("--calculate", "0", code=2).stdout == ""
antipodal = json.loads(run("--calculate", "0", "0", "0", "180").stdout)
assert antipodal["initial_bearing_deg"] is None
assert antipodal["bearing_status"] == "antipodal"

with tempfile.TemporaryDirectory(prefix="hamclock-cli-") as directory:
    path = pathlib.Path(directory) / "station.conf"
    run("--save-station", str(path), "N0CALL", "0", "0", "51.5", "-0.1", "system")
    before = path.read_bytes()
    assert stat.S_IMODE(path.stat().st_mode) == 0o600
    saved = json.loads(run("--load-station", str(path)).stdout)
    assert saved == {"callsign": "N0CALL", "appearance": "system", "de_lat": 0,
                     "de_lon": 0, "dx_lat": 51.5, "dx_lon": -0.1}
    run("--save-station", str(path), "N0CALL", "91", "0", "0", "0", "dark", code=2)
    assert path.read_bytes() == before
    run("--save-station", str(path), "N0CALL", "0", "0", "0", "0", "invalid", code=2)
    assert path.read_bytes() == before
    link = pathlib.Path(directory) / "link.conf"
    link.symlink_to(path)
    run("--save-station", str(link), "N0CALL", "0", "0", "0", "0", "dark", code=2)
    assert link.is_symlink() and path.read_bytes() == before
    run("--load-station", str(link), code=2)
    path.write_text("version=1\n", encoding="ascii")
    run("--load-station", str(path), code=2)
    assert path.read_text(encoding="ascii") == "version=1\n"
print("Executable CLI regression checks passed")
