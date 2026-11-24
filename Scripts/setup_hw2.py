"""Run from UnrealEditor-Cmd with -EnablePlugins=PythonScriptPlugin -run=pythonscript."""

import unreal

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level_editor.load_level("/Game/ThirdPerson/Lvl_ThirdPerson"), "Could not load Lvl_ThirdPerson"
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "HW2.Setup")
actors = {
    actor.get_actor_label(): actor
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
}
assert {"HW2 Guard", "HW2 Navigation", "HW2 Obstacles"} <= actors.keys(), "HW2 setup did not place all actors"
assert actors["HW2 Guard"].get_editor_property("behavior_tree"), "Guard has no Behavior Tree"
assert level_editor.save_current_level(), "Could not save Lvl_ThirdPerson"
