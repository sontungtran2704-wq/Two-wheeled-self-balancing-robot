"""Additional robot parts. Dimensions not supplied by user are explicitly sourced."""
import sys, math, json
from pathlib import Path
import build_robot_parts as b
from sw_part import sketch, sketch_line, sketch_circle, sketch_polygon, sketch_corner_rectangle, extrude_boss, extrude_cut
from sw_connect import mm, get_com_member
sys.path.insert(0,r'C:\Users\USER\.agents\skills\solidworks-automation\subskills\solidworks-threaded-holes\scripts')
from create_threaded_hole_template import ThreadedHoleParams, add_real_thread_feature, add_cosmetic_thread_feature, collect_thread_feature_evidence, create_offset_plane, all_edges, circle_center_radius

def washer():
    old=b.OUT/'05_Vong_dem_M3.SLDPRT'
    for doc in get_com_member(b.session.sw,'GetDocuments') or []:
        if get_com_member(doc,'GetPathName')==str(old): b.session.close(doc)
    m=b.session.new_part()
    with sketch(m,'Front Plane') as sk:
        sketch_circle(m,0,0,mm(3.5)); b.define(m)
    assert extrude_boss(m,sk,mm(.5)) is not None
    with sketch(m,'Front Plane') as sk:
        sketch_circle(m,0,0,mm(1.6)); b.define(m)
    assert extrude_cut(m,sk,0,both_directions=True) is not None
    b.finish(m,'05_Vong_dem_M3',[7,7,.5],{'standard_dimensions':'DIN125A / ISO7089 nominal','outer_mm':7,'inner_mm':3.2,'thickness_mm':.5,'source':'https://xonstorage.z8.web.core.windows.net/pdf/bossard_1887521_apr22_xonlink.pdf'})

def nut():
    m=b.session.new_part()
    radius=5.5/math.sqrt(3)
    with sketch(m,'Front Plane') as sk:
        sketch_polygon(m,0,0,mm(radius),6)
        b.define(m)
    assert extrude_boss(m,sk,mm(2.4)) is not None
    with sketch(m,'Front Plane') as sk:
        sketch_circle(m,0,0,mm(1.25)); b.define(m)
    assert extrude_cut(m,sk,0,both_directions=True) is not None
    params=ThreadedHoleParams('M3x0.5',5.5,5.5,2.4,0,0,3,.5,2.5,2.4,2.4,0,True,'front','6H',True,'never')
    try:
        add_real_thread_feature(m,params)
        representation='real-thread'
    except Exception as exc:
        print('REAL_THREAD_FAILED',str(exc),flush=True)
        add_cosmetic_thread_feature(m,params)
        representation='cosmetic-thread'
    evidence=collect_thread_feature_evidence(m,params,0)
    print('THREAD_EVIDENCE',json.dumps(evidence,default=str),flush=True)
    b.finish(m,'06_Dai_oc_M3',[radius*2,5.5,2.4],{'thread':'M3x0.5 RH','representation':representation,'thread_evidence':evidence,'across_flats_mm':5.5,'height_mm':2.4,'scope':'assembly model; outer edge chamfers omitted; not a manufacturing drawing','source':'https://media.distrelec.com/Web/Downloads/_t/ds/1241613_eng_tds.pdf'})

def screw(length,number):
    m=b.session.new_part()
    with sketch(m,'Front Plane') as sk:
        m.SketchManager.AddToDB=True
        seg=sketch_circle(m,0,0,mm(1.5))
        m.SketchManager.AddToDB=False
        assert seg is not None
        try: b.define(m)
        except RuntimeError:
            m.ClearSelection2(True)
            assert seg.Select4(False,m.SelectionManager.CreateSelectData())
            dim=m.AddDiameterDimension2(mm(5),mm(5),0)
            assert dim is not None
            dim.GetDimension2(0).SystemValue=mm(3)
            b.define(m)
    assert extrude_boss(m,sk,mm(length)) is not None
    with sketch(m,'Front Plane') as sk:
        sketch_circle(m,0,0,mm(2.75)); b.define(m)
    assert extrude_boss(m,sk,mm(3),direction=False) is not None
    plane=create_offset_plane(m,'Mat_dau_vit','Front Plane',3)
    with sketch(m,plane) as sk:
        sketch_polygon(m,0,0,mm(2.5/math.sqrt(3)),6); b.define(m)
    assert extrude_cut(m,sk,mm(1.3),direction=False) is not None
    edge=None
    for e in all_edges(m):
        cr=circle_center_radius(e)
        if cr and abs(cr[1]-mm(1.5))<1e-7 and abs(cr[0][2]+mm(length))<1e-7:
            edge=e; break
    assert edge is not None
    m.ClearSelection2(True)
    assert edge.Select2(False,1)
    d=m.FeatureManager.CreateDefinition(87)
    d.InitializeThreadData()
    for attr,value in [('Edge',edge),('Type','Metric Die'),('ThreadMethod',0),('EndCondition',1),('Revolutions',length/.5),('Size','M3x0.5'),('Pitch',mm(.5)),('PitchOverride',True),('DiameterOverride',False),('RightHanded',True)]:
        setattr(d,attr,value)
    f=m.FeatureManager.CreateFeature(d)
    assert f is not None,'External thread failed'
    f.Name='Ren_ngoai_M3x0_5'
    b.finish(m,f'{number}_Vit_M3x{length}',[5.5,5.5,length+3],{'thread':'M3x0.5 RH','length_under_head_mm':length,'head_diameter_mm':5.5,'head_height_mm':3,'socket_af_mm':2.5,'socket_depth_mm':1.3,'scope':'assembly model; small edge chamfers and underhead fillet omitted','source':'https://media.distrelec.com/Web/Downloads/_t/ds/1003763_eng_tds.pdf'})

def bracket():
    m=b.session.new_part()
    with sketch(m,'Front Plane') as sk:
        assert sketch_corner_rectangle(m,mm(-25),0,mm(25),mm(55)) is not None
        b.define(m)
    assert extrude_boss(m,sk,mm(2)) is not None
    with sketch(m,'Front Plane') as sk:
        m.SketchManager.AddToDB=True
        assert sketch_corner_rectangle(m,mm(-25),0,mm(25),mm(2)) is not None
        m.SketchManager.AddToDB=False
        b.define(m)
    assert extrude_boss(m,sk,mm(45)) is not None
    with sketch(m,'Front Plane') as sk:
        m.SketchManager.AddToDB=True
        for x in (-15.5,15.5):
            for y in (19.5,50.5): assert sketch_circle(m,mm(x),mm(y),mm(1.7)) is not None
        m.SketchManager.AddToDB=False
        b.define(m)
    assert extrude_cut(m,sk,0,both_directions=True) is not None
    b.finish(m,'10_Ga_L_CHUA_DU_LO',[50,55,45],{'height_mm':55,'width_mm':50,'foot_outer_depth_mm':45,'thickness_mm':2,'motor_center_height_mm':35,'motor_holes_pitch_mm':[31,31],'motor_clearance_diameter_mm':3.4,'status':'incomplete: central opening diameter and foot hole longitudinal position pending','scope':'sharp corner simplified bracket; bend radius not specified; not flat pattern'})

def standoff():
    import shutil
    target=b.OUT/'09_Tru_luc_giac_M3_42.SLDPRT'
    if not target.exists(): shutil.copy2(b.OUT/'06_Dai_oc_M3.SLDPRT',target)
    m=b.session.open(str(target),silent=True)
    dim=m.Parameter('D1@Sketch1')
    print('HEX_DIM',float(dim.SystemValue),flush=True)
    dim.SystemValue=mm(4)
    m.Parameter('D1@Boss-Extrude1').SystemValue=mm(42)
    f=m.FeatureByName('Thread_M3x0.5_Internal_Through_RH')
    d=get_com_member(f,'GetDefinition')
    import pythoncom
    from win32com.client import VARIANT
    null=VARIANT(pythoncom.VT_DISPATCH,None)
    assert d.AccessSelections(m,null)
    d.Revolutions=84
    assert f.ModifyDefinition(d,m,null)
    assert m.ForceRebuild3(False)
    b.finish(m,'09_Tru_luc_giac_M3_42',[8/math.sqrt(3),4,42],{'thread':'M3x0.5 RH through','across_flats_mm':4,'length_mm':42,'source':'user measurements','scope':'assembly model; edge chamfers omitted'})

if __name__=='__main__':
    if sys.argv[1]=='washer': washer()
    elif sys.argv[1]=='nut': nut()
    elif sys.argv[1]=='standoff': standoff()
    elif sys.argv[1]=='bracket': bracket()
    elif sys.argv[1]=='save_bracket':
        m=b.session.active_doc
        assert get_com_member(m,'GetPathName').endswith('10_Ga_L_CHUA_DU_LO.SLDPRT')
        print(json.dumps(b.collect_geometry_measurements(m)),flush=True)
        b.finish(m,'10_Ga_L',[50,55,45],{'height_mm':55,'width_mm':50,'foot_outer_depth_mm':45,'thickness_mm':2,'motor_center_height_mm':35,'central_hole_mm':22,'motor_holes_pitch_mm':[31,31],'clearance_diameter_mm':3.4,'foot_hole_spacing_mm':30,'foot_hole_from_outer_edge_mm':22,'foot_hole_model_xz_mm':[[-15,-23],[15,-23]],'scope':'sharp corner simplified bracket; bend radius not specified; not flat pattern'})
    elif sys.argv[1]=='bracket_feet':
        m=b.session.open(str(b.OUT/'10_Ga_L_CHUA_DU_LO.SLDPRT'),silent=True)
        with sketch(m,'Top Plane') as sk:
            m.SketchManager.AddToDB=True
            for x in (-15,15): assert sketch_circle(m,mm(x),mm(23),mm(1.7)) is not None
            m.SketchManager.AddToDB=False
            b.define(m)
        f=extrude_cut(m,sk,0,both_directions=True)
        assert f is not None
        f.Name='Hai_lo_chan_M3'
        volume=get_com_member(get_com_member(m.Extension,'CreateMassProperty'),'Volume')*1e9
        expected=50*55*2+50*2*(45-2)-6*math.pi*1.7**2*2-math.pi*11**2*2
        assert abs(volume-expected)<.2,(volume,expected)
        b.finish(m,'10_Ga_L',[50,55,45],{'height_mm':55,'width_mm':50,'foot_outer_depth_mm':45,'thickness_mm':2,'motor_center_height_mm':35,'central_hole_mm':22,'motor_holes_pitch_mm':[31,31],'clearance_diameter_mm':3.4,'foot_hole_spacing_mm':30,'foot_hole_from_outer_edge_mm':22,'foot_hole_model_xz_mm':[[-15,-23],[15,-23]],'scope':'sharp corner simplified bracket; bend radius not specified; not flat pattern'})
    elif sys.argv[1]=='bracket_center':
        m=b.session.open(str(b.OUT/'10_Ga_L_CHUA_DU_LO.SLDPRT'),silent=True)
        assert m.FeatureByName('Lo_tam_D22') is None
        with sketch(m,'Front Plane') as sk:
            m.SketchManager.AddToDB=True
            assert sketch_circle(m,0,mm(35),mm(11)) is not None
            m.SketchManager.AddToDB=False
            b.define(m)
        f=extrude_cut(m,sk,0,both_directions=True)
        assert f is not None
        f.Name='Lo_tam_D22'
        b.finish(m,'10_Ga_L_CHUA_DU_LO',[50,55,45],{'height_mm':55,'width_mm':50,'foot_outer_depth_mm':45,'thickness_mm':2,'motor_center_height_mm':35,'central_hole_mm':22,'motor_holes_pitch_mm':[31,31],'motor_clearance_diameter_mm':3.4,'foot_hole_spacing_mm':30,'status':'incomplete: foot hole longitudinal position pending','scope':'sharp corner simplified bracket; bend radius not specified; not flat pattern'})
    elif sys.argv[1]=='verify_bracket':
        name='10_Ga_L'
        m=b.session.open(str(b.OUT/(name+'.SLDPRT')),silent=True)
        assert m.ForceRebuild3(False)
        volume=get_com_member(get_com_member(m.Extension,'CreateMassProperty'),'Volume')*1e9
        expected=50*55*2+50*2*(45-2)-6*math.pi*1.7**2*2-math.pi*11**2*2
        assert abs(volume-expected)<.2,(volume,expected)
        geo=b.collect_geometry_measurements(m)
        positions=[(x,y) for x in (-15.5,15.5) for y in (19.5,50.5)]
        check=b.validate_hole_positions(geo,[{'id':str(i),'diameter_mm':3.4,'position_mm':[x,y,0]} for i,(x,y) in enumerate(positions)]+[{'id':'center','diameter_mm':22,'position_mm':[0,35,0]}])
        assert check['status']=='pass',check
        assert geo['hole_count']==7,geo
        foot=[h for h in geo['holes'] if abs(h['axis'][1])>.99]
        assert len(foot)==2
        assert sorted(round(h['position_mm'][0],4) for h in foot)==[-15,15]
        assert all(abs(h['position_mm'][2]+23)<.001 and abs(h['diameter_mm']-3.4)<.001 for h in foot)
        assert b.session.export(m,str(b.REVIEW/(name+'.stl')))
        (b.REVIEW/'Kiem_tra_ga_L.json').write_text(json.dumps({'volume_mm3':volume,'expected_volume_mm3':expected,'motor_holes':check,'geometry':geo,'status':'simplified bracket: all seven holes present; bend radius not specified'},indent=2),encoding='utf-8')
        b.session.close(m)
    elif sys.argv[1]=='fix_threads':
        import pythoncom
        from win32com.client import VARIANT
        null=VARIANT(pythoncom.VT_DISPATCH,None)
        for name,turns in [('06_Dai_oc_M3',4.8),('07_Vit_M3x5',10),('08_Vit_M3x10',20),('09_Tru_luc_giac_M3_42',84)]:
            m=b.session.open(str(b.OUT/(name+'.SLDPRT')),silent=True)
            f=m.FeatureByName('Ren_ngoai_M3x0_5' if name.startswith(('07','08')) else 'Thread_M3x0.5_Internal_Through_RH')
            d=get_com_member(f,'GetDefinition')
            assert d.AccessSelections(m,null)
            d.PitchOverride=True
            d.Pitch=mm(.5)
            d.Revolutions=turns
            assert f.ModifyDefinition(d,m,null)
            assert m.ForceRebuild3(False)
            assert b.session.save(m,str(b.OUT/(name+'.SLDPRT')))
            assert b.session.export(m,str(b.OUT/(name+'.STEP')))
            b.session.close(m)
    elif sys.argv[1]=='screws':
        screw(5,'07')
        screw(10,'08')
    elif sys.argv[1]=='save_nut':
        m=b.session.active_doc
        assert not get_com_member(m,'GetPathName')
        for doc in get_com_member(b.session.sw,'GetDocuments') or []:
            if get_com_member(doc,'GetPathName')==str(b.OUT/'06_Dai_oc_M3.SLDPRT'):
                b.session.close(doc)
        b.finish(m,'06_Dai_oc_M3',[11/math.sqrt(3),5.5,2.4],{'thread':'M3x0.5 RH','representation':'real-thread','across_flats_mm':5.5,'height_mm':2.4,'scope':'assembly model; outer edge chamfers omitted','source':'https://media.distrelec.com/Web/Downloads/_t/ds/1241613_eng_tds.pdf'})
    elif sys.argv[1]=='verify':
        results=[]
        for name in ('05_Vong_dem_M3','06_Dai_oc_M3','07_Vit_M3x5','08_Vit_M3x10','09_Tru_luc_giac_M3_42'):
            m=b.session.open(str(b.OUT/(name+'.SLDPRT')),silent=True)
            assert m.ForceRebuild3(False)
            volume=get_com_member(get_com_member(m.Extension,'CreateMassProperty'),'Volume')*1e9
            result={'name':name,'volume_mm3':volume}
            if name.startswith('05'):
                expected=math.pi*(3.5**2-1.6**2)*.5
                assert abs(volume-expected)<.01,(volume,expected)
                result['expected_volume_mm3']=expected
            else:
                f=m.FeatureByName('Ren_ngoai_M3x0_5' if name.startswith(('07','08')) else 'Thread_M3x0.5_Internal_Through_RH')
                assert f is not None
                d=get_com_member(f,'GetDefinition')
                result['pitch_mm']=float(d.Pitch)*1000
                result['thread_size']=str(d.Size)
                print('THREAD_READBACK',result,flush=True)
                assert abs(result['pitch_mm']-.5)<1e-6
                result['revolutions']=float(d.Revolutions)
                assert abs(result['revolutions']-({'06':4.8,'07':10,'08':20,'09':84}[name[:2]]))<1e-5
                if name.startswith('06'): assert 40<volume<52,volume
                if name.startswith('09'): assert 250<volume<380,volume
            assert b.session.export(m,str(b.REVIEW/(name+'.stl')))
            results.append(result)
            b.session.close(m)
        (b.REVIEW/'Kiem_tra_bo_sung.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
        print(json.dumps(results),flush=True)
