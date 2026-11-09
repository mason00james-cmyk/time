"""
SECOND HAND - level builder
===========================
Builds the whole level (8 rooms, lights, colors, turrets, guards, signs, exit) from scratch.
Re-running it deletes everything it built last time and builds it again, so:

    edit a number below  ->  re-run  ->  press Play  ->  commit

Run it from the Unreal editor's Cmd box (bottom of the screen):
    py "C:/Users/mason/UnrealProjects/Time/Scripts/build_level.py"

Rule: tweak THIS FILE, not the level by hand. Anything the script made gets wiped on the next run.
(Things you add by hand yourself are left alone.)
"""

import math
import unreal

# =====================================================================================
# TUNING - change these and re-run
# =====================================================================================

MAP_PATH = "/Game/SecondHand/Maps/L_SecondHand"
MAT_DIR = "/Game/SecondHand/Materials"

WALL_HEIGHT = 450          # cm
WALL_THICK = 40
DOOR_WIDTH = 320

# Colors (hex). Crab Game-style bright pastels.
COLORS = {
    "floor":   "#8FE3F0",
    "wall":    "#F2F4F7",
    "trim":    "#FFFFFF",
    "yellow":  "#FFD966",
    "mint":    "#9FE8B5",
    "pink":    "#FF9EB5",
    "turret":  "#3A3F4B",
    "goal":    "#7CFF9B",
    "floor2":  "#6CCFE0",
    "blue":    "#8EC5FF",
    "screen":  "#1F2430",
    "crate":   "#E8A15C",
    "rusher":  "#2F6BFF",   # blue suit
    "pistol":  "#FF3B30",   # red suit
    "shotgun": "#FF8A00",   # orange suit
}

# Roof + indoor lights (a roof blocks the sun, so each room gets ceiling lights)
ROOF = True
ROOF_THICK = 20
ROOM_LIGHT_LUMENS = 12000      # brightness of each ceiling light
ROOM_LIGHT_SPACING = 800       # cm between ceiling lights
ROOM_LIGHT_SHADOWS = True      # turn off if the game runs slow

# Detail
DETAIL = True              # checker floors, wall stripes, baseboards, light panels, pillars, crates, screens
TILE = 300                 # checker tile size (cm)
ACCENTS = ["yellow", "mint", "pink", "blue"]   # each room gets the next accent color

SKY_FOG_COLOR = "#5BC8F5"
SUN_INTENSITY = 7.0        # lux
SATURATION = 1.15
BLOOM = 0.3

# Guard model tweaks (if guards face sideways, change GUARD_MESH_YAW by 90 or -90)
GUARD_HEIGHT = 188.0       # cm - the model is auto-scaled to this height
PISTOL_LENGTH = 26.0       # cm - guns are auto-scaled to these lengths
SHOTGUN_LENGTH = 95.0
GUARD_MESH_YAW = -90.0
GUARD_MESH_Z = -94.0
GUARD_SIGHT = 2000.0
GUN_OFFSET = (0.0, 0.0, 0.0)      # gun position relative to the right hand
GUN_ROTATION = (0.0, 0.0, 0.0)    # (roll, pitch, yaw)

# Rooms, in order. length = along the hall (X), width = side to side (Y). All in cm.
ROOMS = [
    {"name": "Test Chamber",    "length": 2400, "width": 700,  "sign": "ROUND 1: STAND STILL. THINK."},
    {"name": "Break Room",      "length": 1600, "width": 1400, "sign": "ROUND 2: LEFT CLICK TO PUNCH"},
    {"name": "Security Desk",   "length": 1800, "width": 1400, "sign": "ROUND 3: DODGE, THEN PUNCH"},
    {"name": "Ambush Corridor", "length": 2000, "width": 600,  "sign": "ROUND 4: HOLD R TO REWIND"},
    {"name": "Human Bowling",   "length": 1800, "width": 600,  "sign": "ROUND 5: HUMAN BOWLING"},
    {"name": "Bullet Gauntlet", "length": 2800, "width": 1000, "sign": "ROUND 6: THE GAUNTLET"},
    {"name": "Crossfire",       "length": 1600, "width": 1600, "sign": "ROUND 7: CROSSFIRE"},
    {"name": "Server Vault",    "length": 2400, "width": 1600, "sign": "FINAL ROUND: THE VAULT"},
]

# =====================================================================================
# Plumbing (you shouldn't need to touch below here unless adding new kinds of things)
# =====================================================================================

TAG = "SH_Gen"
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
BASE_MAT = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")


def log(msg):
    unreal.log("[SecondHand] " + msg)


def warn(msg):
    unreal.log_warning("[SecondHand] " + msg)


def hex_to_linear(h):
    h = h.lstrip("#")
    srgb = [int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)]
    return unreal.LinearColor(*[c ** 2.2 for c in srgb], 1.0)


def vec(x, y, z):
    return unreal.Vector(float(x), float(y), float(z))


def rot(roll=0.0, pitch=0.0, yaw=0.0):
    return unreal.Rotator(roll=float(roll), pitch=float(pitch), yaw=float(yaw))


# ---------------- level ----------------

def open_or_create_level():
    if EAL.does_asset_exist(MAP_PATH):
        LES.load_level(MAP_PATH)
        log("Loaded " + MAP_PATH)
    else:
        LES.new_level(MAP_PATH)
        log("Created " + MAP_PATH)


def clear_generated():
    n = 0
    for a in EAS.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            EAS.destroy_actor(a)
            n += 1
    log("Removed %d old generated actors" % n)


def tag(actor, label, folder):
    actor.set_actor_label(label)
    actor.set_editor_property("tags", [TAG])
    try:
        actor.set_folder_path("SecondHand/" + folder)
    except Exception:
        pass
    return actor


# ---------------- materials ----------------

MATS = {}


def flat_material(name, hex_color):
    path = MAT_DIR + "/MI_" + name
    if EAL.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        mi = tools.create_asset("MI_" + name, MAT_DIR, unreal.MaterialInstanceConstant,
                                unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, BASE_MAT)
    MEL.set_material_instance_vector_parameter_value(mi, "Color", hex_to_linear(hex_color))
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi


def build_materials():
    for k, v in COLORS.items():
        MATS[k] = flat_material(k, v)


# ---------------- primitives ----------------

def box(label, center, size, mat="wall", folder="Geometry", rotation=None):
    a = EAS.spawn_actor_from_class(unreal.StaticMeshActor, vec(*center), rotation or rot())
    smc = a.get_editor_property("static_mesh_component")
    smc.set_static_mesh(CUBE)
    smc.set_material(0, MATS[mat])
    a.set_actor_scale3d(vec(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return tag(a, label, folder)


def wall(label, x0, y0, x1, y1, height=WALL_HEIGHT, mat="wall", folder="Geometry", trim=True):
    """Axis-aligned wall from (x0,y0) to (x1,y1) on the floor."""
    cx, cy = (x0 + x1) / 2.0, (y0 + y1) / 2.0
    sx = max(abs(x1 - x0), WALL_THICK)
    sy = max(abs(y1 - y0), WALL_THICK)
    box(label, (cx, cy, height / 2.0), (sx, sy, height), mat, folder)
    if trim:
        box(label + "_Trim", (cx, cy, height + 6), (sx + 10, sy + 10, 12), "trim", folder)


def sign(text, loc, yaw, size=60, color="#FFFFFF", screen=True):
    if screen and DETAIL:
        # dark "monitor" behind the text; pushed back toward the wall (+X when facing -X)
        back = 1 if yaw == 180 else -1
        w = len(text) * size * 0.62 + 80
        box("Screen_" + text[:12], (loc[0] + back * 6, loc[1], loc[2]), (6, w, size * 1.9), "screen", "Signs")
    a = EAS.spawn_actor_from_class(unreal.TextRenderActor, vec(*loc), rot(yaw=yaw))
    try:
        comp = a.get_editor_property("text_render")
        comp.set_text(text)
        comp.set_world_size(size)
        comp.set_text_render_color(unreal.Color(*[int(color.lstrip("#")[i:i + 2], 16) for i in (0, 2, 4)], 255))
        comp.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        comp.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    except Exception as e:
        warn("Sign setup issue: %s" % e)
    return tag(a, "Sign_" + text[:20], "Signs")


# ---------------- gameplay actors ----------------

ASSET_PATHS = None


def all_assets():
    global ASSET_PATHS
    if ASSET_PATHS is None:
        ASSET_PATHS = list(EAL.list_assets("/Game", recursive=True, include_folder=False))
    return ASSET_PATHS


def find_anim(word):
    """Find an AnimSequence whose name contains `word` and that belongs to the guard's skeleton."""
    for path in all_assets():
        short = path.split("/")[-1].split(".")[0]
        if word.lower() not in short.lower():
            continue
        asset = unreal.load_asset(path)
        if not asset or asset.get_class().get_name() != "AnimSequence":
            continue
        try:
            skel = asset.get_editor_property("skeleton")
            if skel is None or "guard" not in skel.get_name().lower():
                continue
        except Exception:
            continue
        log("Using animation %s for %s" % (short, word))
        return asset
    warn("No '%s' animation found - guards will T-pose. Re-import the new SK_Guard.fbx." % word)
    return None


def find_asset(name, cls_name=None):
    for path in all_assets():
        if path.split("/")[-1].split(".")[0] == name:
            asset = unreal.load_asset(path)
            if asset and (cls_name is None or asset.get_class().get_name() == cls_name):
                return asset
    return None


GUARD_MESH = None
IDLE_ANIM = None
RUN_ANIM = None
PISTOL_MESH = None
SHOTGUN_MESH = None


GUARD_SCALE = 1.0
GUN_SCALES = {}


def fit_scale(asset, target, label):
    """Scale factor that makes the asset's longest side equal target (cm)."""
    try:
        e = asset.get_bounds().box_extent
        size = max(e.x, e.y, e.z) * 2.0
        sc = target / size if size > 0.01 else 1.0
        log("%s imported %.1f cm long -> scaling by %.4f" % (label, size, sc))
        return sc
    except Exception as ex:
        warn("Couldn't measure %s (%s) - using scale 1" % (label, ex))
        return 1.0


def load_guard_assets():
    global GUARD_MESH, PISTOL_MESH, SHOTGUN_MESH, GUARD_SCALE, IDLE_ANIM, RUN_ANIM
    IDLE_ANIM = find_anim("Idle")
    RUN_ANIM = find_anim("Run")
    GUARD_MESH = find_asset("SK_Guard", "SkeletalMesh")
    if GUARD_MESH:
        try:
            h = GUARD_MESH.get_bounds().box_extent.z * 2.0
            GUARD_SCALE = GUARD_HEIGHT / h if h > 1.0 else 1.0
            log("SK_Guard imported %.1f cm tall -> scaling guards by %.4f" % (h, GUARD_SCALE))
        except Exception as e:
            warn("Couldn't measure SK_Guard (%s) - using scale 1" % e)
    PISTOL_MESH = find_asset("SM_Pistol", "StaticMesh")
    SHOTGUN_MESH = find_asset("SM_Shotgun", "StaticMesh")
    if PISTOL_MESH:
        GUN_SCALES["pistol"] = fit_scale(PISTOL_MESH, PISTOL_LENGTH, "SM_Pistol")
    if SHOTGUN_MESH:
        GUN_SCALES["shotgun"] = fit_scale(SHOTGUN_MESH, SHOTGUN_LENGTH, "SM_Shotgun")
    if not GUARD_MESH:
        warn("SK_Guard not found - import SK_Guard.fbx (Skeletal Mesh + Create Physics Asset). Guards will be invisible.")
    if not PISTOL_MESH:
        warn("SM_Pistol not found - guards will have no visible gun.")


def turret(label, loc, yaw, interval=1.0, delay=0.0, aim=False, spread=0.0):
    if not hasattr(unreal, "SHTurret"):
        warn("SHTurret class missing - did the C++ build succeed?")
        return None
    t = EAS.spawn_actor_from_class(unreal.SHTurret, vec(*loc), rot(yaw=yaw))
    t.set_editor_property("fire_interval", float(interval))
    t.set_editor_property("start_delay", float(delay))
    t.set_editor_property("aim_at_player", bool(aim))
    t.set_editor_property("spread", float(spread))
    try:
        t.get_editor_property("body").set_material(0, MATS["turret"])
    except Exception:
        pass
    return tag(t, label, "Turrets")


def guard(label, loc, kind="PISTOL", yaw=180.0, active=True):
    if not hasattr(unreal, "SHGuard"):
        warn("SHGuard class missing - did the Stage 3 C++ build succeed?")
        return None
    g = EAS.spawn_actor_from_class(unreal.SHGuard, vec(loc[0], loc[1], loc[2] + 100), rot(yaw=yaw))
    g.set_editor_property("guard_type", getattr(unreal.SHGuardType, kind))
    g.set_editor_property("sight_range", float(GUARD_SIGHT))
    g.set_editor_property("active", bool(active))
    if IDLE_ANIM:
        g.set_editor_property("idle_anim", IDLE_ANIM)
    if RUN_ANIM:
        g.set_editor_property("run_anim", RUN_ANIM)

    mesh = g.get_editor_property("mesh")
    if GUARD_MESH:
        try:
            mesh.set_skeletal_mesh_asset(GUARD_MESH)
        except Exception:
            mesh.set_editor_property("skeletal_mesh_asset", GUARD_MESH)
        mesh.set_relative_location_and_rotation(vec(0, 0, GUARD_MESH_Z), rot(yaw=GUARD_MESH_YAW), False, False)
        mesh.set_relative_scale3d(vec(GUARD_SCALE, GUARD_SCALE, GUARD_SCALE))
        try:
            names = [str(n) for n in mesh.get_material_slot_names()]
            suit = [i for i, n in enumerate(names) if "suit" in n.lower()]
            if suit:
                mesh.set_material(mesh.get_material_index(names[suit[0]]), MATS[kind.lower()])
            else:
                warn("No 'Suit' material slot found on SK_Guard (slots: %s)" % names)
        except Exception as e:
            warn("Suit color issue: %s" % e)

    gun = g.get_editor_property("gun_mesh")
    gun_asset = SHOTGUN_MESH if (kind == "SHOTGUN" and SHOTGUN_MESH) else PISTOL_MESH
    if gun_asset and kind != "RUSHER":
        gun.set_static_mesh(gun_asset)
        gun.set_relative_location_and_rotation(vec(*GUN_OFFSET), rot(*GUN_ROTATION), False, False)
        # The hand bone carries a hidden scale from Blender, so make the gun's scale absolute (ignore the bone)
        gs = GUN_SCALES.get("shotgun" if gun_asset == SHOTGUN_MESH else "pistol", 1.0)
        try:
            gun.set_absolute(False, False, True)
        except Exception as e:
            warn("set_absolute failed: %s" % e)
        gun.set_relative_scale3d(vec(gs, gs, gs))
    return tag(g, label, "Guards")


def goal(loc):
    if hasattr(unreal, "SHGoal"):
        a = EAS.spawn_actor_from_class(unreal.SHGoal, vec(loc[0], loc[1], loc[2] + 150), rot())
        try:
            a.get_editor_property("pad").set_material(0, MATS["goal"])
        except Exception:
            pass
        tag(a, "Exit_Elevator", "Gameplay")
    else:
        warn("SHGoal class missing (Stage 5 C++ not built yet) - placing a plain marker instead.")
        box("Exit_Marker", (loc[0], loc[1], 5), (300, 300, 10), "goal", "Gameplay")


# ---------------- lighting / look ----------------

def room_light(label, loc, radius):
    a = EAS.spawn_actor_from_class(unreal.PointLight, vec(*loc), rot())
    try:
        lc = a.get_editor_property("light_component")
        lc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        lc.set_intensity(ROOM_LIGHT_LUMENS)
        lc.set_attenuation_radius(radius)
        lc.set_light_color(hex_to_linear("#FFF8EC"))
        lc.set_cast_shadows(ROOM_LIGHT_SHADOWS)
    except Exception as e:
        warn("Room light issue: %s" % e)
    return tag(a, label, "Lighting")


def build_lighting():
    sun = EAS.spawn_actor_from_class(unreal.DirectionalLight, vec(0, 0, 1000), rot(pitch=-45, yaw=35))
    lc = sun.get_editor_property("light_component")
    lc.set_intensity(SUN_INTENSITY)
    lc.set_light_color(hex_to_linear("#FFF4E0"))
    try:
        lc.set_editor_property("atmosphere_sun_light", True)
    except Exception:
        pass
    tag(sun, "Sun", "Lighting")

    tag(EAS.spawn_actor_from_class(unreal.SkyAtmosphere, vec(0, 0, 0), rot()), "SkyAtmosphere", "Lighting")

    sky = EAS.spawn_actor_from_class(unreal.SkyLight, vec(0, 0, 800), rot())
    try:
        sc = sky.get_editor_property("light_component")
        sc.set_mobility(unreal.ComponentMobility.MOVABLE)
        sc.set_editor_property("real_time_capture", True)
        sc.set_intensity(1.2)
    except Exception as e:
        warn("Sky light issue: %s" % e)
    tag(sky, "SkyLight", "Lighting")

    fog = EAS.spawn_actor_from_class(unreal.ExponentialHeightFog, vec(0, 0, 0), rot())
    try:
        fc = fog.get_editor_property("component")
        fc.set_fog_density(0.02)
        fc.set_editor_property("fog_inscattering_luminance", hex_to_linear(SKY_FOG_COLOR))
    except Exception as e:
        warn("Fog issue: %s" % e)
    tag(fog, "Fog", "Lighting")

    pp = EAS.spawn_actor_from_class(unreal.PostProcessVolume, vec(0, 0, 0), rot())
    pp.set_editor_property("unbound", True)
    s = pp.get_editor_property("settings")
    try:
        s.set_editor_property("override_color_saturation", True)
        s.set_editor_property("color_saturation", unreal.Vector4(SATURATION, SATURATION, SATURATION, 1.0))
        s.set_editor_property("override_bloom_intensity", True)
        s.set_editor_property("bloom_intensity", BLOOM)
        pp.set_editor_property("settings", s)
    except Exception as e:
        warn("Post process issue: %s" % e)
    tag(pp, "PostProcess", "Lighting")


# ---------------- rooms ----------------

def build_rooms():
    x = 0.0
    starts = []
    for r in ROOMS:
        starts.append(x)
        x += r["length"]
    total = x

    # Floor (one long slab) + outer side walls per room
    for i, r in enumerate(ROOMS):
        x0, L, W = starts[i], r["length"], r["width"]
        box("Floor_%d" % (i + 1), (x0 + L / 2, 0, -10), (L, W + WALL_THICK * 2, 20), "floor", "Room%d" % (i + 1))
        wall("WallL_%d" % (i + 1), x0, W / 2 + WALL_THICK / 2, x0 + L, W / 2 + WALL_THICK / 2, folder="Room%d" % (i + 1))
        wall("WallR_%d" % (i + 1), x0, -W / 2 - WALL_THICK / 2, x0 + L, -W / 2 - WALL_THICK / 2, folder="Room%d" % (i + 1))

    # Roof + ceiling lights
    for i, r in enumerate(ROOMS):
        x0, L, W = starts[i], r["length"], r["width"]
        if ROOF:
            box("Roof_%d" % (i + 1), (x0 + L / 2, 0, WALL_HEIGHT + ROOF_THICK / 2),
                (L + WALL_THICK, W + WALL_THICK * 2, ROOF_THICK), "wall", "Room%d" % (i + 1))
        count = max(1, int(round(L / ROOM_LIGHT_SPACING)))
        for n in range(count):
            lx = x0 + L * (n + 0.5) / count
            room_light("Light_%d_%d" % (i + 1, n), (lx, 0, WALL_HEIGHT - 60), max(L / count, W) * 1.2)
            if ROOF and DETAIL:
                box("LightPanel_%d_%d" % (i + 1, n), (lx, 0, WALL_HEIGHT - 4), (220, 120, 8), "trim", "Room%d" % (i + 1))

    # Back wall of room 1, front wall of last room
    W0 = ROOMS[0]["width"]
    wall("Wall_Start", 0, -W0 / 2 - WALL_THICK, 0, W0 / 2 + WALL_THICK, folder="Room1")
    Wn = ROOMS[-1]["width"]
    wall("Wall_End", total, -Wn / 2 - WALL_THICK, total, Wn / 2 + WALL_THICK, folder="Room%d" % len(ROOMS))

    # Dividers with a door that alternates sides (so guards can't see straight through)
    door_side = {}
    for i in range(len(ROOMS) - 1):
        xd = starts[i + 1]
        Wa, Wb = ROOMS[i]["width"], ROOMS[i + 1]["width"]
        half = max(Wa, Wb) / 2 + WALL_THICK
        inner = min(Wa, Wb) / 2
        side = 1 if i % 2 == 0 else -1
        door_c = side * (inner - DOOR_WIDTH / 2 - 40)
        door_side[i + 1] = (side, door_c)
        lo, hi = door_c - DOOR_WIDTH / 2, door_c + DOOR_WIDTH / 2
        wall("Divider_%d_a" % (i + 1), xd, -half, xd, lo, folder="Room%d" % (i + 1))
        wall("Divider_%d_b" % (i + 1), xd, hi, xd, half, folder="Room%d" % (i + 1))
        # Room i's sign goes on its far wall (faces the player walking +X), away from the door
        sign_y = -side * min(Wa / 4, 250)
        sign(ROOMS[i]["sign"], (xd - WALL_THICK / 2 - 2, sign_y, 300), 180, 50)

    sign(ROOMS[-1]["sign"], (total - WALL_THICK / 2 - 2, 0, 380), 180, 50)
    if DETAIL:
        build_detail(starts, door_side)
    return starts, door_side


def build_detail(starts, door_side):
    for i, r in enumerate(ROOMS):
        x0, L, W = starts[i], r["length"], r["width"]
        f = "Room%d" % (i + 1)
        accent = ACCENTS[i % len(ACCENTS)]

        # Checker floor (only the darker squares are extra pieces)
        nx, ny = int(math.ceil(L / TILE)), int(math.ceil(W / TILE))
        for ix in range(nx):
            for iy in range(ny):
                if (ix + iy) % 2 == 0:
                    continue
                xa, xb = x0 + ix * TILE, min(x0 + (ix + 1) * TILE, x0 + L)
                ya, yb = -W / 2 + iy * TILE, min(-W / 2 + (iy + 1) * TILE, W / 2)
                box("Tile_%d_%d_%d" % (i + 1, ix, iy), ((xa + xb) / 2, (ya + yb) / 2, 1), (xb - xa, yb - ya, 2), "floor2", f)

        # Baseboards + a colored stripe along both side walls
        for side in (1, -1):
            y = side * (W / 2 - 4)
            box("Baseboard_%d_%d" % (i + 1, side), (x0 + L / 2, y, 12), (L, 8, 24), accent, f)
            box("Stripe_%d_%d" % (i + 1, side), (x0 + L / 2, side * (W / 2 - 3), 150), (L, 6, 36), accent, f)

        # Pillars in the big open rooms (not the gauntlet / vault, where cover would change the puzzle)
        if W >= 1400 and i not in (5, 7):
            for fx in (0.22, 0.78):
                for fy in (-0.32, 0.32):
                    px, py = x0 + L * fx, W * fy
                    box("Pillar_%d_%.0f_%.0f" % (i + 1, fx * 100, fy * 100), (px, py, WALL_HEIGHT / 2), (90, 90, WALL_HEIGHT), "wall", f)
                    box("PillarBand_%d_%.0f_%.0f" % (i + 1, fx * 100, fy * 100), (px, py, 150), (96, 96, 36), accent, f)

        # A couple of crates in the corner by the entrance (away from the door)
        if i in (1, 2, 6):
            side = -door_side[i][0] if i in door_side else 1
            cy = side * (W / 2 - 110)
            box("Crate_%d_a" % (i + 1), (x0 + 130, cy, 60), (120, 120, 120), "crate", f)
            box("Crate_%d_b" % (i + 1), (x0 + 260, cy, 45), (90, 90, 90), "crate", f)
            box("Crate_%d_c" % (i + 1), (x0 + 140, cy, 160), (80, 80, 80), "crate", f, rotation=rot(yaw=20))


def populate(starts, door_side):
    S = starts

    # Player start
    ps = EAS.spawn_actor_from_class(unreal.PlayerStart, vec(150, 0, 100), rot(yaw=0))
    tag(ps, "PlayerStart", "Gameplay")

    # ---- Room 1: Test Chamber - turrets fire across the hall, staggered
    W = ROOMS[0]["width"]
    for n, xo in enumerate([600, 950, 1300, 1650, 2000]):
        side = 1 if n % 2 == 0 else -1
        turret("R1_Turret_%d" % n, (xo, side * (W / 2 - 30), 140 if n % 2 else 110),
               yaw=-90 * side, interval=2.0, delay=n * 0.4)

    # ---- Room 2: Break Room - one rusher, a table
    x0, L = S[1], ROOMS[1]["length"]
    box("R2_Table", (x0 + L / 2, 0, 40), (300, 150, 80), "yellow", "Room2")
    guard("R2_Rusher", (x0 + L - 300, -200, 0), "RUSHER")

    # ---- Room 3: Security Desk - one pistol guard behind a desk
    x0, L = S[2], ROOMS[2]["length"]
    box("R3_Desk", (x0 + L - 500, 0, 55), (100, 400, 110), "mint", "Room3")
    guard("R3_Pistol", (x0 + L - 300, 0, 0), "PISTOL")

    # ---- Room 4: Ambush Corridor - guard on a balcony behind the entrance
    x0, L, W = S[3], ROOMS[3]["length"], ROOMS[3]["width"]
    side, door_c = door_side[3]
    by = -side * (W / 2 - 150)
    box("R4_Balcony", (x0 + 200, by, 240), (300, 300, 20), "pink", "Room4")
    box("R4_BalconyPost", (x0 + 200, by, 115), (60, 60, 230), "pink", "Room4")
    box("R4_BalconyRail", (x0 + 340, by, 280), (20, 300, 60), "trim", "Room4")
    guard("R4_Ambusher", (x0 + 200, by, 250), "PISTOL", yaw=0)

    # ---- Room 5: Human Bowling - three guards in a line
    x0, L = S[4], ROOMS[4]["length"]
    for n, xo in enumerate([1000, 1180, 1360]):
        guard("R5_Pin_%d" % n, (x0 + xo, 0, 0), "PISTOL", active=(n == 0))

    # ---- Room 6: Bullet Gauntlet - raised walkway, two shooters, one turret, a ramp
    x0, L, W = S[5], ROOMS[5]["length"], ROOMS[5]["width"]
    wx = x0 + L - 450
    box("R6_Walkway", (wx + 200, 0, 240), (500, W, 20), "yellow", "Room6")
    box("R6_WalkwayBase", (wx + 200, 0, 115), (500, W, 230), "wall", "Room6")
    ramp_y = W / 2 - 170
    run, rise = 700.0, 250.0
    ang = math.degrees(math.atan2(rise, run))
    box("R6_Ramp", (wx - run / 2 + 20, ramp_y, rise / 2 - 5), (math.hypot(run, rise), 300, 20), "mint", "Room6",
        rotation=rot(pitch=ang))
    lo, hi = -W / 2, ramp_y - 150
    box("R6_Rail", (wx - 40, (lo + hi) / 2, 280), (20, hi - lo, 60), "trim", "Room6")
    guard("R6_Shooter_a", (wx + 150, -250, 250), "PISTOL")
    guard("R6_Shooter_b", (wx + 300, 150, 250), "PISTOL")
    turret("R6_Turret", (x0 + L - 60, -100, 330), yaw=180, interval=1.2, aim=True, spread=4.0)

    # ---- Room 7: Crossfire - two shotgun guards on opposite sides
    x0, L, W = S[6], ROOMS[6]["length"], ROOMS[6]["width"]
    guard("R7_Shotgun_a", (x0 + L / 2, W / 2 - 150, 0), "SHOTGUN", yaw=-90)
    guard("R7_Shotgun_b", (x0 + L / 2 + 200, -W / 2 + 150, 0), "SHOTGUN", yaw=90)

    # ---- Room 8: Server Vault - racks, six hidden guards, the exit
    x0, L, W = S[7], ROOMS[7]["length"], ROOMS[7]["width"]
    for row, xo in enumerate([500, 950, 1400]):
        for col, yo in enumerate([-450, 0, 450]):
            box("R8_Rack_%d_%d" % (row, col), (x0 + xo, yo, 175), (120, 280, 350), "turret", "Room8")
    hidden = [(620, -450, "PISTOL"), (620, 450, "RUSHER"), (1070, 0, "SHOTGUN"),
              (1070, 450, "PISTOL"), (1520, -450, "RUSHER"), (1520, 450, "PISTOL")]
    for n, (xo, yo, kind) in enumerate(hidden):
        guard("R8_Guard_%d" % n, (x0 + xo, yo, 0), kind)
    goal((x0 + L - 250, 0, 0))
    sign("EXIT", (x0 + L - WALL_THICK / 2 - 2, 0, 330), 180, 90, "#1E8C45")


# =====================================================================================

def main():
    open_or_create_level()
    with unreal.ScopedEditorTransaction("Build Second Hand level"):
        clear_generated()
        build_materials()
        load_guard_assets()
        build_lighting()
        starts, door_side = build_rooms()
        populate(starts, door_side)
    LES.save_current_level()
    log("Done! Press Play. (Level: %s)" % MAP_PATH)


main()
