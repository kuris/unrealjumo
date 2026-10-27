"""Build explicit mobile PBR materials from the original embedded GLB textures."""
import json
import struct
from pathlib import Path
import unreal


def main():
    root=Path(unreal.Paths.project_dir()).resolve()
    report=[]
    for name in ['seonbi','pojol']:
        source=root/'Source/glb'/(name+'_final_rigged.glb')
        with source.open('rb') as stream:
            stream.read(12)
            length,_=struct.unpack('<II',stream.read(8))
            doc=json.loads(stream.read(length))
            length,_=struct.unpack('<II',stream.read(8))
            binary=stream.read(length)
        material_data=doc['materials'][0]
        pbr=material_data['pbrMetallicRoughness']
        indices={'BaseColor':pbr['baseColorTexture']['index'],'Normal':material_data['normalTexture']['index'],'MetallicRoughness':pbr['metallicRoughnessTexture']['index']}
        folder='/Game/Characters/'+name+'_final_rigged/Mobile'
        textures={}
        for semantic,index in indices.items():
            image=doc['images'][doc['textures'][index]['source']]
            view=doc['bufferViews'][image['bufferView']]
            start=view.get('byteOffset',0)
            extension='.jpg' if image['mimeType']=='image/jpeg' else '.png'
            extracted=root/'Source/CharacterTextures'/(name+'_'+semantic+extension)
            extracted.parent.mkdir(parents=True,exist_ok=True)
            extracted.write_bytes(binary[start:start+view['byteLength']])
            asset_name='T_'+name+'_'+semantic
            path=folder+'/'+asset_name
            texture=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
            if texture is None:
                task=unreal.AssetImportTask()
                task.set_editor_properties({'filename':str(extracted),'destination_path':folder,'destination_name':asset_name,'automated':True,'save':True,'replace_existing':False})
                unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
                texture=unreal.load_asset(path)
            if not isinstance(texture,unreal.Texture2D):
                raise RuntimeError('Texture import failed: '+path)
            texture.modify()
            texture.set_editor_property('srgb',semantic=='BaseColor')
            texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP if semantic=='Normal' else (unreal.TextureCompressionSettings.TC_MASKS if semantic=='MetallicRoughness' else unreal.TextureCompressionSettings.TC_DEFAULT))
            unreal.EditorAssetLibrary.save_loaded_asset(texture,False)
            textures[semantic]=texture
        matpath=folder+'/M_'+name+'_Mobile'
        material=unreal.load_asset(matpath) if unreal.EditorAssetLibrary.does_asset_exist(matpath) else unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_'+name+'_Mobile',folder,unreal.Material,unreal.MaterialFactoryNew())
        material.modify()
        material.set_editor_properties({'two_sided':True,'used_with_skeletal_mesh':True,'shading_model':unreal.MaterialShadingModel.MSM_DEFAULT_LIT,'blend_mode':unreal.BlendMode.BLEND_OPAQUE})
        lib=unreal.MaterialEditingLibrary
        lib.delete_all_material_expressions(material)
        nodes={}
        for i,(semantic,texture) in enumerate(textures.items()):
            node=lib.create_material_expression(material,unreal.MaterialExpressionTextureSample,-400,i*240)
            node.set_editor_property('texture',texture)
            node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if semantic=='Normal' else (unreal.MaterialSamplerType.SAMPLERTYPE_MASKS if semantic=='MetallicRoughness' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR))
            nodes[semantic]=node
        lib.connect_material_property(nodes['BaseColor'],'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
        lib.connect_material_property(nodes['Normal'],'RGB',unreal.MaterialProperty.MP_NORMAL)
        lib.connect_material_property(nodes['MetallicRoughness'],'G',unreal.MaterialProperty.MP_ROUGHNESS)
        lib.connect_material_property(nodes['MetallicRoughness'],'B',unreal.MaterialProperty.MP_METALLIC)
        lib.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material,False):raise RuntimeError('Material save failed')
        mesh=unreal.load_asset('/Game/Characters/'+name+'_final_rigged/'+name+'_final_rigged')
        mesh.modify()
        slots=mesh.get_editor_property('materials')
        slots[0].set_editor_property('material_interface',material)
        mesh.call_method('SetMaterials',(slots,))
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False):raise RuntimeError('Mesh save failed')
        report.append({'character':name,'material':material.get_path_name(),'textures':{k:v.get_path_name() for k,v in textures.items()}})
        unreal.log('CHARACTER_MOBILE_MATERIAL_PASS: '+name)
    (root/'Saved/JumoAnimations/mobile_materials.json').write_text(json.dumps(report,indent=2))


if __name__=='__main__':main()
