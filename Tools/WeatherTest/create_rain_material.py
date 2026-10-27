"""Run inside Unreal Python to generate the isolated mobile rain material."""
import unreal


def main():
    path = "/Game/Weather/M_ClearRain"
    material = unreal.load_asset(path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_ClearRain", "/Game/Weather", unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Rain material path contains an unexpected asset: " + path)
    # Unlit translucency keeps rain from becoming brown/black in shadow on mobile.
    # No refraction or scene-color sampling: thin falling water stays inexpensive.
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    library = unreal.MaterialEditingLibrary
    library.delete_all_material_expressions(material)
    tint = library.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -320, 0)
    tint.set_editor_property("constant", unreal.LinearColor(0.75, 0.85, 0.95, 1.0))
    opacity = library.create_material_expression(material, unreal.MaterialExpressionConstant, -320, 160)
    opacity.set_editor_property("r", 0.14)
    library.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    library.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    library.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Could not save " + path)
    unreal.log("WEATHER_RAIN_MATERIAL_PASS: translucent unlit water, opacity 0.14; " + path)


if __name__ == "__main__":
    main()
