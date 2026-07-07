"""
Autonomous OutpostHD play agent — drives the game via logs/automation.cmd (kyle.23+).
No keyboard/mouse required; watch the game window while the agent plays.
"""

from __future__ import annotations

import re
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

GAME_EXE = Path(
    r"C:\Users\h4don\Games\OPHD\dist\OutpostHD-KylesColony-v0.8.10-kyle.23-win64\appOPHD.exe"
)
SESSION_LOG = Path(r"C:\Users\h4don\AppData\Roaming\LairWorks\OutpostHD\logs\session.log")
AUTOMATION_CMD = Path(r"C:\Users\h4don\AppData\Roaming\LairWorks\OutpostHD\logs\automation.cmd")
AUTOMATION_STATUS = Path(r"C:\Users\h4don\AppData\Roaming\LairWorks\OutpostHD\logs\automation.status")
PLAY_LOG = Path(r"C:\Users\h4don\Games\OPHD\dist\play-journal.txt")

GOALS = {
    "population": 500,
    "food_buffer": 200,
    "research_total": 69,
}


@dataclass
class ColonyState:
    turn: int = 0
    pop: int = 0
    food: int = 0
    research_done: int = 0
    research_active: int = 0
    research_queued: int = 0
    resources: int = 0
    structures: int = 0


def log(msg: str) -> None:
    line = f"[{time.strftime('%H:%M:%S')}] {msg}"
    print(line, flush=True)
    with PLAY_LOG.open("a", encoding="utf-8") as f:
        f.write(line + "\n")


def session_log_offset() -> int:
    return SESSION_LOG.stat().st_size if SESSION_LOG.exists() else 0


def read_log_since(offset: int) -> str:
    if not SESSION_LOG.exists():
        return ""
    with SESSION_LOG.open("r", encoding="utf-8", errors="ignore") as handle:
        handle.seek(offset)
        return handle.read()


def parse_colony_state(text: str) -> ColonyState | None:
    matches = re.findall(
        r"turn=(\d+).*pop=(\d+).*structures=(\d+).*research_completed=(\d+)"
        r".*research_active=(\d+).*research_queued=(\d+).*resources=(\d+).*food=(\d+)",
        text,
    )
    if not matches:
        return None
    t = matches[-1]
    return ColonyState(
        turn=int(t[0]),
        pop=int(t[1]),
        structures=int(t[2]),
        research_done=int(t[3]),
        research_active=int(t[4]),
        research_queued=int(t[5]),
        resources=int(t[6]),
        food=int(t[7]),
    )


def read_colony_state_since(offset: int = 0) -> ColonyState | None:
    return parse_colony_state(read_log_since(offset))


def colony_session_started_since(offset: int) -> bool:
    return "--- Session begin:" in read_log_since(offset)


def wait_for_colony_loaded(offset: int, timeout: float = 120.0) -> ColonyState | None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        state = read_colony_state_since(offset)
        if state and state.turn > 0:
            return state
        if colony_session_started_since(offset):
            return ColonyState(turn=0)
        time.sleep(0.5)
    return read_colony_state_since(offset)


def send_commands(commands: list[str]) -> None:
    AUTOMATION_CMD.parent.mkdir(parents=True, exist_ok=True)
    for _ in range(50):
        if not AUTOMATION_CMD.exists():
            break
        time.sleep(0.05)
    body = "\n".join(commands) + "\n"
    tmp = AUTOMATION_CMD.with_suffix(".cmd.tmp")
    for attempt in range(8):
        try:
            tmp.write_text(body, encoding="utf-8")
            tmp.replace(AUTOMATION_CMD)
            return
        except PermissionError:
            time.sleep(0.1 * (attempt + 1))
        except OSError:
            time.sleep(0.1 * (attempt + 1))
    AUTOMATION_CMD.write_text(body, encoding="utf-8")


def read_automation_status() -> str:
    if not AUTOMATION_STATUS.exists():
        return ""
    return AUTOMATION_STATUS.read_text(encoding="utf-8", errors="ignore").strip()


def plan_commands(state: ColonyState) -> list[str]:
    commands: list[str] = []

    if state.food < GOALS["food_buffer"]:
        log(f"FOOD CRISIS turn {state.turn}: food={state.food} — orderpizza")
        commands.append("cheat orderpizza")

    if (
        state.research_active == 0
        and state.research_queued == 0
        and state.research_done < GOALS["research_total"]
    ):
        commands.append("research_queue")

    if state.pop < GOALS["population"] and state.food >= GOALS["food_buffer"]:
        commands.extend(["cheat gettowork", "cheat smartypants"])

    batch = 25 if state.food > 100 and state.turn > 0 else 5
    commands.append(f"turn {batch}")
    return commands


def find_running_game_pid() -> int | None:
    result = subprocess.run(
        [
            "powershell",
            "-NoProfile",
            "-Command",
            "(Get-Process appOPHD -ErrorAction SilentlyContinue | Select-Object -First 1).Id",
        ],
        capture_output=True,
        text=True,
    )
    text = result.stdout.strip()
    return int(text) if text.isdigit() else None


def is_pid_alive(pid: int) -> bool:
    result = subprocess.run(
        ["powershell", "-NoProfile", "-Command", f"Get-Process -Id {pid} -ErrorAction SilentlyContinue"],
        capture_output=True,
        text=True,
    )
    return result.returncode == 0


def goal_summary(state: ColonyState) -> str:
    return (
        f"turn={state.turn} pop={state.pop}/{GOALS['population']} "
        f"food={state.food} research={state.research_done}/{GOALS['research_total']} "
        f"(active={state.research_active} queued={state.research_queued}) res={state.resources}"
    )


def play_forever() -> None:
    append = PLAY_LOG.exists() and PLAY_LOG.stat().st_size > 0
    with PLAY_LOG.open("a" if append else "w", encoding="utf-8") as f:
        f.write(f"=== Play session {'resumed' if append else 'started'} {time.strftime('%Y-%m-%d %H:%M:%S')} (file bridge) ===\n")

    attached_pid = find_running_game_pid()
    proc: subprocess.Popen | None = None
    resumed = attached_pid is not None

    if resumed:
        log(f"Resuming — attached to running game PID {attached_pid}")
        state = read_colony_state_since(0) or ColonyState()
        log_offset = session_log_offset()
    else:
        log("Launching OutpostHD kyle.23 with autosave...")
        if not GAME_EXE.exists():
            log(f"ERROR: missing exe: {GAME_EXE}")
            sys.exit(1)
        log_offset = session_log_offset()
        proc = subprocess.Popen(
            [str(GAME_EXE), "autosave"],
            cwd=str(GAME_EXE.parent),
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP,
        )
        attached_pid = proc.pid
        log(f"PID {attached_pid}")
        state = wait_for_colony_loaded(log_offset, timeout=90.0)
        if not state and not colony_session_started_since(log_offset):
            log("ERROR: colony did not load")
            proc.kill()
            sys.exit(1)
        time.sleep(2.0)
        state = read_colony_state_since(log_offset) or state
        if state.turn == 0:
            send_commands(["turn 1"])
            time.sleep(2.0)
            state = read_colony_state_since(log_offset) or state

    log("Colony ready — play loop via automation.cmd")

    log(f"Starting: {goal_summary(state)}")
    log("Strategy: food → research queue → population → batch turns")

    last_turn = state.turn
    idle_cycles = 0

    while (proc is None or proc.poll() is None) and is_pid_alive(attached_pid):
        state = read_colony_state_since(0) or state
        commands = plan_commands(state)
        try:
            send_commands(commands)
        except (PermissionError, OSError) as exc:
            log(f"WARN: command write failed ({exc}) — retrying next cycle")
        time.sleep(1.5 if "turn 25" in commands[-1] else 0.8)

        status = read_automation_status()
        if status:
            log(f"Game ack: {status}")

        new_state = read_colony_state_since(0)
        if new_state:
            state = new_state
            if state.turn != last_turn:
                idle_cycles = 0
                last_turn = state.turn
                if state.turn % 10 == 0:
                    log(f"Progress: {goal_summary(state)}")
            else:
                idle_cycles += 1

        if idle_cycles >= 10:
            log("No turn progress — nudging with turn 1")
            send_commands(["escape", "turn 1"])
            idle_cycles = 0

        if (
            state.research_done >= GOALS["research_total"]
            and state.pop >= GOALS["population"]
            and state.food >= GOALS["food_buffer"]
        ):
            log("Primary goals met — continuing...")

    log("Play agent stopped.")


if __name__ == "__main__":
    play_forever()