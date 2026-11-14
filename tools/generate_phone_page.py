"""Embed the actual desktop dashboard and its local dependencies for ESP32.

Run after editing desktop HTML/CSS/JS, the phone transport, or design.json.
The generated header is self-contained: Arduino IDE needs no generator at upload.
"""
from pathlib import Path
import gzip, hashlib, json, re

ROOT = Path(__file__).resolve().parents[1]
PUBLIC = ROOT / '4_Web/public'
FIRMWARE = ROOT / '03_Firmware_ESP32_S3/balancebot'
BUILD = ROOT / '.build'

def generate():
    inputs = {}
    def read(path):
        data = path.read_bytes()
        inputs[str(path.relative_to(ROOT)).replace('\\','/')] = hashlib.sha256(data).hexdigest()
        return data.decode('utf-8')
    html = read(PUBLIC/'index.html')
    css = read(PUBLIC/'style.css')
    dashboard = read(PUBLIC/'dashboard.js')
    feedback = read(PUBLIC/'button-feedback.js')
    adapter = read(ROOT/'4_Web/phone-adapter.js')
    chart = read(ROOT/'4_Web/node_modules/chart.js/dist/chart.min.js')
    chart_license = read(ROOT/'4_Web/node_modules/chart.js/LICENSE.md')
    drive_html = read(ROOT/'4_Web/phone-drive.html')
    drive_css = read(ROOT/'4_Web/phone-drive.css')
    drive_js = read(ROOT/'4_Web/phone-drive.js')
    welcome_html=read(PUBLIC/'welcome.html')
    welcome_css=read(PUBLIC/'welcome.css')
    welcome_js=read(PUBLIC/'welcome.js')
    welcome_js=welcome_js.split('// Render QR code for mobile connection')[0]
    welcome_html=welcome_html[:welcome_html.index('<div id="phone-connect-box"')]+welcome_html[welcome_html.index('</section></main>'):]
    welcome_html=welcome_html.replace('<script src="/qrcode.min.js"></script>','').replace('Tài khoản lưu trên máy chạy web.','Tài khoản lưu trên xe ESP32.')
    # The direct-phone page has no phone-QR button/modal or QR dependency.
    html = re.sub(r'\s*<button id="btn-phone-qr".*?</button>', '', html, flags=re.S)
    html = re.sub(r'\s*<!-- QR MODAL FOR PHONE CONNECTION -->.*?(?=\s*<script src="button-feedback.js")', '', html, flags=re.S)
    preset=html.index('<h3 class="section-title">📋 Cấu Hình Sẵn</h3>')
    start=html.rfind('<div class="sidebar-section">',0,preset)
    end=html.index('<div class="sidebar-section">',preset)
    html=html[:start]+html[end:]
    html=html.replace('<p id="reset-status" class="hint" role="status">','<p id="reset-status" hidden class="hint" role="status">')
    design_text = read(ROOT/'shared/design.json')
    design = json.loads(design_text)
    robot = (ROOT/'shared/robot.json').read_bytes()
    assert hashlib.sha256(robot).hexdigest() == design['config_sha256'], 'Stale design.json'
    inputs['shared/robot.json'] = hashlib.sha256(robot).hexdigest()
    solver = read(ROOT/'4_Web/lqr.js')
    solver = solver[solver.index('function calculate('):solver.index('module.exports')]
    solver = solver.replace('function calculate(', 'window.phoneCalculate = function(')
    solver = solver.replace('const d=loadDesign()', 'const d=window.PHONE_DESIGN')
    def script(text): return '<script>\n'+text.replace('</script','<\\/script')+'\n</script>'
    auth_transport="""const authNativeFetch=window.fetch.bind(window);window.fetch=(url,opt={})=>{if(url==='/api/auth/login'||url==='/api/auth/register'){const p=JSON.parse(opt.body);if(new TextEncoder().encode(p.password).length>128)return Promise.resolve(new Response(JSON.stringify({error:'Mật khẩu tối đa 128 byte.'}),{status:400}));opt={...opt,headers:{'Content-Type':'text/plain'},body:p.username+'\\n'+p.password};}return authNativeFetch(url,{...opt,signal:AbortSignal.timeout(10000)});};"""
    welcome_html=welcome_html.replace('<link rel="stylesheet" href="/welcome.css">','<style>'+welcome_css+'</style>').replace('<script src="/welcome.js"></script>',script(auth_transport+welcome_js))
    for tag, content in [
        ('<link rel="stylesheet" href="style.css">','<style>'+css+'\n'+drive_css+'</style>'),
        ('<script src="/socket.io/socket.io.js"></script>',script('window.PHONE_DESIGN='+json.dumps(design,separators=(',',':'))+';\n'+solver+adapter)),
        ('<script src="/qrcode.min.js"></script>',''),
        ('<script src="/vendor/chart/chart.min.js"></script>',script(chart)),
        ('<script src="button-feedback.js"></script>',drive_html+script(feedback)),
        ('<script src="dashboard.js"></script>',script(dashboard)+script(drive_js)),
        ('<script src="file-entry.js"></script>',''),
        ('<script src="/session.js"></script>',''),
    ]:
        assert html.count(tag)==1, tag
        html = html.replace(tag,content)
    html = re.sub(r"\s*<script>window.Chart \|\| document.write\(.*?</script>", '', html)
    assert not re.search(r'<script[^>]+src=',html), 'An external script remains'
    assert 'cdnjs.cloudflare.com' not in html, 'Online fallback remains'
    html = html.replace('<html lang="vi">','<html lang="vi">\n<!-- Bundled Chart.js license:\n'+chart_license+'\n-->')
    raw = html.encode('utf-8'); compressed=gzip.compress(raw,compresslevel=9,mtime=0)
    icon=read(PUBLIC/'icons/icon.svg').encode('utf-8')
    manifest=read(PUBLIC/'manifest.json').encode('utf-8')
    output=['// Generated by tools/generate_phone_page.py. Do not edit.','#pragma once','#include <cstddef>','namespace phone {']
    welcome_compressed=gzip.compress(welcome_html.encode('utf-8'),compresslevel=9,mtime=0)
    for name,data in [('page',compressed),('welcome',welcome_compressed),('icon',icon),('manifest',manifest)]:
        output += [f'static const unsigned char {name}[]={{']
        output += [','.join(f'0x{x:02x}' for x in data[i:i+24])+',' for i in range(0,len(data),24)]
        output += ['};',f'static constexpr size_t {name}Size=sizeof {name};']
    output += ['}']
    (FIRMWARE/'PhonePage.h').write_text('\n'.join(output)+'\n',encoding='utf-8')
    BUILD.mkdir(exist_ok=True)
    (BUILD/'phone-page.html.gz').write_bytes(compressed)
    (BUILD/'phone-welcome.html.gz').write_bytes(welcome_compressed)
    (BUILD/'phone-page-inputs.json').write_text(json.dumps({'inputs':inputs,'uncompressed_bytes':len(raw),'gzip_bytes':len(compressed),'page_sha256':hashlib.sha256(raw).hexdigest(),'welcome_gzip_bytes':len(welcome_compressed),'welcome_sha256':hashlib.sha256(welcome_html.encode('utf-8')).hexdigest()},indent=2),encoding='utf-8')
    assert gzip.decompress(compressed)==raw
    print(f'PASS: shared desktop dashboard + phone driving view, QR excluded, {len(raw)} bytes -> {len(compressed)} gzip bytes; readback verified')

if __name__=='__main__': generate()
