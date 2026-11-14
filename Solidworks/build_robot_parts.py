import sys, json, math
import pythoncom
from win32com.client import VARIANT
from pathlib import Path
sys.path.insert(0, r'C:\Users\USER\.agents\skills\solidworks-automation\scripts')
from sw_session import SolidWorksSession
from sw_connect import mm, get_com_member
from sw_part import sketch, sketch_rectangle, sketch_circle, sketch_line, sketch_arc, extrude_boss, extrude_cut
from sw_sketch_quality import fully_define_sketch, inspect_model_sketches, inspect_sketch
from sw_review import collect_geometry_measurements, run_review, validate_hole_positions

OUT = Path(__file__).parent / 'Bo_part_robot_2026-11-13'
OUT.mkdir(exist_ok=True)
REVIEW = OUT / 'Kiem_tra'
REVIEW.mkdir(exist_ok=True)
session = SolidWorksSession()
print('CONNECTED', session.connection_info, flush=True)

def define(model):
    try:
        return fully_define_sketch(model)
    except RuntimeError:
        record=inspect_sketch(model.SketchManager.ActiveSketch)
        # A circle can be fully driven by relations to an already dimensioned slot.
        if record['constrained_status']==3 and record['fix_count']==0 and not record['errors']:
            print('RELATION_DRIVEN_SKETCH',record,flush=True)
            return record
        raise

def finish(model, name, expected, notes):
    assert model.ForceRebuild3(False), 'Rebuild failed'
    native = OUT / (name + '.SLDPRT')
    assert session.save(model, str(native)), 'Save failed'
    assert session.export(model, str(OUT / (name + '.STEP'))), 'STEP failed'
    session.close(model)
    model = session.open(str(native), silent=True)
    measured = collect_geometry_measurements(model)
    env = measured['envelope_mm']
    extrema=[]
    bodies=list(get_com_member(model,'GetBodies2',0,False))
    assert len(bodies)==1
    for axis in range(3):
        ends=[]
        for sign in (-1,1):
            direction=[0.,0.,0.]; direction[axis]=float(sign)
            result=bodies[0].GetExtremePoint(*direction,0.,0.,0.)
            assert isinstance(result,tuple) and len(result)==4 and result[0],result
            ends.append(result[axis+1]*1000)
        extrema.append(ends[1]-ends[0])
    actual=extrema
    assert all(abs(a-b)<0.05 for a,b in zip(actual, expected)), (actual,expected)
    quality = inspect_model_sketches(model)
    report, _ = run_review(model, str(REVIEW), basename=name, views=('isometric','front'), expected_outputs=[str(native), str(OUT/(name+'.STEP'))])
    evidence = {'notes':notes,'expected_envelope_mm':expected,'exact_extrema_envelope_mm':actual,'geometry':measured,'sketches':quality,'review':report.get('evaluation')}
    (REVIEW/(name+'_evidence.json')).write_text(json.dumps(evidence,ensure_ascii=False,indent=2,default=str),encoding='utf-8')
    print('SAVED',name,actual,report.get('evaluation'),flush=True)
    session.close(model)

def plate(name, inner_offset=None, reuse=False):
    model = session.active_doc if reuse else session.new_part()
    if reuse:
        assert get_com_member(model,'GetTitle') == 'Part1' and not get_com_member(model,'GetPathName'), 'Unexpected active document'
    else:
        with sketch(model,'Front Plane') as sk:
            sketch_rectangle(model,0,0,mm(70),mm(125))
            define(model)
        f=extrude_boss(model,sk,mm(2)); assert f is not None
    positions=[(x,y) for x in (-30,30) for y in (-55,-37.5,37.5,55)]
    if inner_offset is not None:
        positions += [(x,y) for x in (-12.5,12.5) for y in (-62.5+inner_offset,62.5-inner_offset)]
    with sketch(model,'Front Plane') as sk:
        for x,y in positions: sketch_circle(model,mm(x),mm(y),mm(1.7))
        define(model)
    f=extrude_cut(model,sk,0,both_directions=True); assert f is not None
    f.Name='Lo_xuyen_M3_D3_4'
    # Separate cuts avoid fragile multi-arc contour selection in the COM wrapper.
    with sketch(model,'Front Plane') as sk:
        for x in (-25,25):
            for y in (-17.5,17.5):
                sketch_rectangle(model,mm(x),mm(y),mm(5),mm(15))
        define(model)
    f=extrude_cut(model,sk,0,both_directions=True); assert f is not None
    f.Name='Than_ranh'
    with sketch(model,'Front Plane') as sk:
        for x in (-25,25):
            for y in (-25,-10,10,25): sketch_circle(model,mm(x),mm(y),mm(2.5))
        define(model)
    f=extrude_cut(model,sk,0,both_directions=True); assert f is not None
    f.Name='Dau_ranh_R2_5'
    measured=collect_geometry_measurements(model)
    check=validate_hole_positions(measured,[{'id':str(i),'diameter_mm':3.4,'position_mm':[x,y,0]} for i,(x,y) in enumerate(positions)])
    assert check['status']=='pass',check
    finish(model,name,[70,125,2],{'hole_diameter_design_choice_mm':3.4,'hole_centers_xy_mm':positions,'slots_total_length_mm':20,'slots_width_mm':5,'slot_centers_xy_mm':[(x,y) for x in (-25,25) for y in (-17.5,17.5)],'status':'complete geometry' if inner_offset is not None or name.startswith('01') else 'missing four inner holes'})

def references():
    model=session.new_part()
    with sketch(model,'Front Plane') as sk:
        sketch_circle(model,0,0,mm(33))
        sketch_circle(model,0,0,mm(6))
        define(model)
    assert extrude_boss(model,sk,mm(25)) is not None
    finish(model,'03_Banh_xe_THAM_KHAO',[66,66,25],{'status':'simplified reference only','outside_diameter_mm':66,'width_mm':25,'bore_mm':12,'omitted':'tread, spokes, hub protrusion, exact rim/tire interface'})
    model=session.new_part()
    with sketch(model,'Front Plane') as sk:
        sketch_rectangle(model,0,0,mm(42),mm(42)); define(model)
    assert extrude_boss(model,sk,mm(34)) is not None
    with sketch(model,'Front Plane') as sk:
        sketch_circle(model,0,0,mm(2.5)); define(model)
    assert extrude_boss(model,sk,mm(25),direction=False) is not None
    finish(model,'04_Dong_co_THAM_KHAO',[42,42,59],{'status':'simplified reference only','body_mm':[42,42,34],'shaft_diameter_mm':5,'shaft_protrusion_mm':25,'omitted':'D flat, pilot boss diameter, mounting threads, body seams, connector, internals'})

if __name__=='__main__':
    mode=sys.argv[1]
    if mode=='upper': plate('01_Tam_nhom_tren',reuse=True)
    elif mode=='lower': plate('02_Tam_nhom_duoi',float(sys.argv[2]))
    elif mode=='references': references()
    elif mode=='continue':
        m=session.open(str(OUT/'01_Tam_nhom_tren.SLDPRT'),silent=True)
        finish(m,'01_Tam_nhom_tren',[70,125,2],{'status':'complete geometry','hole_diameter_design_choice_mm':3.4})
        plate('02_Tam_nhom_duoi',20)
        references()
    elif mode=='verify':
        results=[]
        for name,count in [('01_Tam_nhom_tren',8),('02_Tam_nhom_duoi',12)]:
            m=session.open(str(OUT/(name+'.SLDPRT')),silent=True)
            volume=get_com_member(get_com_member(m.Extension,'CreateMassProperty'),'Volume')*1e9
            expected=(70*125-count*math.pi*1.7**2-4*(15*5+math.pi*2.5**2))*2
            assert abs(volume-expected)<0.1,(name,volume,expected)
            geom=collect_geometry_measurements(m)
            assert len(geom['holes'])==count,(name,len(geom['holes']))
            arcs=[c for c in geom['cylindrical_faces'] if abs(c['diameter_mm']-5)<0.001]
            assert len(arcs)==8,(name,len(arcs))
            results.append({'part':name,'actual_volume_mm3':volume,'expected_volume_mm3':expected,'round_holes':count,'slot_end_arcs':len(arcs),'status':'pass'})
            session.close(m)
        (REVIEW/'Kiem_tra_doc_lap.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
        print(json.dumps(results),flush=True)
    elif mode=='previews':
        from sw_appearance import set_document_appearance
        from sw_review import save_preview, activate_model_for_preview
        import time
        for path in sorted(OUT.glob('*.SLDPRT')):
            m=session.open(str(path),silent=True)
            activate_model_for_preview(m)
            for body in get_com_member(m,'GetBodies2',0,False) or []:
                body.HideBody(False)
            set_document_appearance(m,'#A5ABB5' if path.name.startswith(('01','02')) else '#50565E')
            m.ViewDisplayShaded()
            m.ForceRebuild3(False)
            m.GraphicsRedraw2()
            session.save(m,str(path))
            time.sleep(1)
            for view in ('front','isometric'):
                save_preview(m,str(REVIEW/(path.stem+'_'+view+'.bmp')),view)
            print('PREVIEW',path.name,flush=True)
            session.close(m)
    elif mode=='mesh':
        for path in sorted(OUT.glob('*.SLDPRT')):
            m=session.open(str(path),silent=True)
            assert session.export(m,str(REVIEW/(path.stem+'.stl')))
            session.close(m)
            print('MESH',path.name,flush=True)
    elif mode=='plates':
        active=session.active_doc
        if active is not None and get_com_member(active,'GetTitle')=='Part1' and not get_com_member(active,'GetPathName'): session.close(active)
        plate('01_Tam_nhom_tren')
        plate('02_Tam_nhom_duoi',20)
    elif mode=='inspect':
        m=session.active_doc
        print(json.dumps(inspect_model_sketches(m),default=str),flush=True)
        print(run_review(m,str(REVIEW),basename='debug',views=('front','isometric'))[1],flush=True)
    elif mode=='recover':
        m=session.active_doc
        if m.SketchManager.ActiveSketch is not None: m.SketchManager.InsertSketch(True)
        f=extrude_cut(m,'Sketch3',0,direction=False)
        print('CUT', f is not None,flush=True)
        assert f is not None
        f.Name='Bon_ranh_20x5'
        finish(m,'01_Tam_nhom_tren',[70,125,2],{'status':'complete geometry','hole_diameter_design_choice_mm':3.4})
