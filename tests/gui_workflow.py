#!/usr/bin/env python3
"""Desktop workflow regression using real X11 input (Xwayland on niri supported).
Run: python3 tests/gui_workflow.py build/orbit_simulator
Requires a running desktop, libX11, libXtst. Writes a screenshot to /tmp.
Widget rectangles come from the optional UI-state file; input is sent through X11/Xwayland.
Maintainer reading order and module ownership: docs/architecture.md.
"""
import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

x = ctypes.CDLL('libX11.so.6')
xt = ctypes.CDLL('libXtst.so.6')
x.XOpenDisplay.argtypes = [ctypes.c_char_p]
x.XOpenDisplay.restype = ctypes.c_void_p
display = x.XOpenDisplay(None)
assert display, 'A running X11/Xwayland desktop is required'
x.XStringToKeysym.argtypes = [ctypes.c_char_p]
x.XStringToKeysym.restype = ctypes.c_ulong
x.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
x.XKeysymToKeycode.restype = ctypes.c_uint
x.XFlush.argtypes = [ctypes.c_void_p]
x.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
xt.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
xt.XTestFakeMotionEvent.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_ulong]
x.XTranslateCoordinates.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_ulong, ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_ulong)]
xt.XTestFakeButtonEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]

def press(name, down):
    symbol = x.XStringToKeysym(name.encode())
    assert symbol, name
    code = x.XKeysymToKeycode(display, symbol)
    assert code, name
    xt.XTestFakeKeyEvent(display, code, int(down), 0)
    x.XSync(display, 0)

def key(name):
    press(name, True)
    time.sleep(0.05)
    press(name, False)
    time.sleep(0.12)

def text(value):
    for char in value:
        if char.isupper():
            press('Shift_L', True)
        key({' ': 'space', '.': 'period', '-': 'minus', '+': 'plus'}.get(char, char.lower()))
        if char.isupper():
            press('Shift_L', False)

def button(number, down):
    xt.XTestFakeButtonEvent(display, number, int(down), 0)
    x.XFlush(display)
    time.sleep(0.035)

with tempfile.TemporaryDirectory(prefix='orbitsim-ui-workflow-') as folder:
    folder = Path(folder)
    state_path = folder / 'ui.json'
    before = json.loads(subprocess.check_output(['niri', 'msg', '-j', 'windows'])) if os.environ.get('NIRI_SOCKET') else []
    env = dict(os.environ, ORBITSIMLITE_UI_STATE=str(state_path), ORBITSIMLITE_DATA_DIR=str(folder / 'data'))
    executable = str(Path(sys.argv[1] if len(sys.argv) > 1 else 'build/orbit_simulator').resolve())
    stderr = open(folder / 'stderr', 'w+')
    process = subprocess.Popen([executable], cwd=folder, env=env, stderr=stderr)
    window = None
    try:
        for _ in range(100):
            if state_path.exists():
                break
            assert process.poll() is None, 'Application exited during startup'
            time.sleep(0.05)
        assert state_path.exists(), 'Editor did not become ready'
        if before:
            previous_ids = {w['id'] for w in before}
        else:
            previous_ids = set()
        if os.environ.get('NIRI_SOCKET'):
            windows = json.loads(subprocess.check_output(['niri', 'msg', '-j', 'windows']))
            window = next(w for w in windows if w['id'] not in previous_ids and w['title'] == 'Apsilume 3D Editor')
            subprocess.run(['niri', 'msg', 'action', 'focus-window', '--id', str(window['id'])], check=True, stdout=subprocess.DEVNULL)
        time.sleep(1.0)
        # Resolve the application's X11 client, rather than the transient focus
        # proxy exposed while a Wayland compositor animates a new window.
        x.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
        x.XDefaultRootWindow.restype = ctypes.c_ulong
        x.XQueryTree.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)), ctypes.POINTER(ctypes.c_uint)]
        x.XFetchName.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_char_p)]
        x.XFree.argtypes = [ctypes.c_void_p]
        def find_client(parent):
            root, owner, children, count = ctypes.c_ulong(), ctypes.c_ulong(), ctypes.POINTER(ctypes.c_ulong)(), ctypes.c_uint()
            if not x.XQueryTree(display, parent, ctypes.byref(root), ctypes.byref(owner), ctypes.byref(children), ctypes.byref(count)):
                return None
            ids = [children[i] for i in range(count.value)]
            if children:
                x.XFree(children)
            for child in ids:
                title = ctypes.c_char_p()
                x.XFetchName(display, child, ctypes.byref(title))
                name = title.value.decode(errors='replace') if title.value else ''
                if title:
                    x.XFree(title)
                if name == 'Apsilume 3D Editor':
                    return child
            for child in ids:
                found = find_client(child)
                if found:
                    return found
            return None
        client = find_client(x.XDefaultRootWindow(display))
        assert client, 'Editor X11 window was not found'
        def state():
            return json.loads(state_path.read_text())
        def wait_state(predicate, message, timeout=3):
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                current = state()
                if predicate(current):
                    return current
                time.sleep(0.03)
            raise AssertionError(f'{message}: {state()}')
        def move(a, b):
            for _ in range(40):
                root_x, root_y, child = ctypes.c_int(), ctypes.c_int(), ctypes.c_ulong()
                x.XTranslateCoordinates(display, client, x.XDefaultRootWindow(display), 0, 0, ctypes.byref(root_x), ctypes.byref(root_y), ctypes.byref(child))
                assert xt.XTestFakeMotionEvent(display, 0, root_x.value + round(a), root_y.value + round(b), 0)
                x.XFlush(display)
                time.sleep(0.05)
                point = state().get("pointer", {})
                if abs(point.get("x", -10000) - round(a)) < 2 and abs(point.get("y", -10000) - round(b)) < 2:
                    break
            else:
                raise AssertionError(f"Pointer did not reach {(a,b)}: {state().get('pointer')}")
        def click(widget):
            rect = state()['widgets'][widget]
            move(rect['x'] + rect['w'] / 2, rect['y'] + rect['h'] / 2)
            button(1, True)
            button(1, False)
            time.sleep(0.16)
        def scroll(ticks):
            widgets = state()['widgets']
            right = max(w['x'] + w['w'] for w in widgets.values())
            move(right - 12, 470)
            for _ in range(abs(ticks)):
                button(5 if ticks > 0 else 4, True)
                button(5 if ticks > 0 else 4, False)
        def reveal(widget):
            for _ in range(24):
                if widget in state()['widgets']:
                    return
                scroll(1)
            raise AssertionError(f'Cannot find {widget}: {state()}')
        def edit(widget, value):
            reveal(widget)
            click(widget)
            wait_state(lambda current: current['editing'], 'Field did not receive focus')
            text(value)
            key('Return')
            wait_state(lambda current: not current['editing'] or current['error'], 'Field did not commit')
            assert not state()['error'], state()['error']
        def screenshot(path):
            if window:
                subprocess.run(['niri', 'msg', 'action', 'screenshot-window', '--id', str(window['id']), '--path', path], check=True, stdout=subprocess.DEVNULL)

        assert state()['dialog'] == 1
        click('scene_2')
        click('startup_csv')
        click('dialog_primary')
        assert state()['dialog'] == 0 and 'Figure' in state()['scene'], state()
        click('play')
        assert state()['paused']
        paused = state()['time']
        time.sleep(0.2)
        assert state()['time'] == paused
        click('step')
        assert abs(state()['time'] - paused - state()['dt']) < 1e-10
        dt = state()['dt']
        click('faster')
        assert state()['speed'] == 2 and state()['dt'] == dt
        selected = state()['selected']['id']
        click('panel_body_' + str(selected))
        click('panel_visible_' + str(selected))
        assert not state()['gizmo'], 'Hidden body still has visible move handles'
        click('panel_visible_' + str(selected))
        assert state()['gizmo']
        edit('property_5', '0.25')
        edit('property_8', '0.125')
        assert state()['selected']['z'] == 0.25 and state()['selected']['vz'] == 0.125
        assert state()['dirty']
        # Invalid input stays editable with a visible error, and cannot corrupt physics.
        edit_mass = 'property_1'
        click(edit_mass)
        text('-1')
        key('Return')
        assert state()['error']
        key('Escape')
        # World-space Z movement through the visible blue handle.
        key('F1')
        handles = state()['gizmo']
        handle = next(h for h in handles if h['axis'] == 2)
        z_before = state()['selected']['z']
        a = (handle['x0'] + 0.75 * (handle['x1'] - handle['x0']), handle['y0'] + 0.75 * (handle['y1'] - handle['y0']))
        move(*a)
        button(1, True)
        move(a[0] + 0.25 * (handle['x1'] - handle['x0']), a[1] + 0.25 * (handle['y1'] - handle['y0']))
        button(1, False)
        assert state()['selected']['z'] > z_before, 'Blue handle did not move Z'
        key('h')
        assert not state()['grid']
        key('g')
        assert not state()['origin']
        click('prediction')
        assert state()['prediction']
        time.sleep(0.3)
        for _ in range(6):
            key('F6')
        horizon = state()['prediction_horizon']
        forecast_range = state()['forecast_range']
        key('F7')
        time.sleep(0.3)
        assert state()['forecast_range'] == forecast_range * 2, (forecast_range, state())
        assert state()['prediction_horizon'] >= horizon
        key('p')
        assert not state()['prediction']
        # Numerical settings have their own section and validation.
        reveal('section_2')
        click('section_2')
        edit('property_9', '0.002')
        assert state()['dt'] == 0.002 and state()['speed'] == 2
        reveal('panel_grid')
        click('panel_grid')
        assert state()['grid']
        key('h')
        assert not state()['grid']
        reveal('panel_origin')
        click('panel_origin')
        assert state()['origin']
        key('g')
        assert not state()['origin']
        # Save, overwrite, and reopen use the same buttons and shortcuts.
        click('save')
        text('Workflow Scene')
        key('Return')
        assert state()['dialog'] == 0 and not state()['dirty'], state()
        saved = folder / 'data/scenes/Workflow Scene.json'
        data = json.loads(saved.read_text())
        body = next(b for b in data['bodies'] if b['id'] == selected)
        assert body['position']['z'] > 0.25 and body['velocity']['z'] == 0.125
        key('k')
        key('Return')
        assert state()['dialog'] == 4
        click('dialog_primary')
        assert state()['dialog'] == 0
        click('load')
        saved_index = sorted(p.name for p in (folder / 'data/scenes').glob('*.json')).index('Workflow Scene.json')
        while 'saved_' + str(saved_index) not in state()['widgets']:
            click('saved_next')
        click('saved_' + str(saved_index))
        click('dialog_primary')
        assert state()['scene'] == 'Workflow Scene' and state()['dt'] == 0.002
        # Unsaved replacements require a deliberate decision.
        key('space')
        key('Page_Up')
        click('reset')
        assert state()['dialog'] == 6
        click('dialog_cancel')
        assert state()['dirty']
        click('help')
        assert state()['dialog'] == 7
        click('dialog_primary')
        assert state()['dialog'] == 0
        screenshot('/tmp/orbitsim-product-editor.png')
        time.sleep(0.3)
        key('Escape')
        wait_state(lambda current: current['dialog'] == 6, 'Unsaved-exit dialog did not open')
        click('discard')
        process.wait(timeout=8)
        stderr.seek(0)
        errors = stderr.read()
        assert 'OpenGL error' not in errors, errors
        assert process.returncode == 0
        assert list((folder / 'data/exports').glob('*.csv'))
        print('GUI workflow passed: chooser, playback, XYZ fields and drag, validation, dt/speed independence, toggles, prediction, save/overwrite/load, unsaved protection.')
    finally:
        if process.poll() is None:
            if window:
                subprocess.run(['niri', 'msg', 'action', 'screenshot-window', '--id', str(window['id']), '--path', '/tmp/orbitsim-product-failure.png'], stdout=subprocess.DEVNULL)
            print('Last UI state:', state_path.read_text() if state_path.exists() else 'unavailable', file=sys.stderr)
            process.terminate()
            process.wait(timeout=5)
        stderr.close()
