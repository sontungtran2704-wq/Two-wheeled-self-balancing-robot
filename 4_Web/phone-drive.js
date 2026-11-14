'use strict';
// Phone-only view/controller. Uses the existing adapter and controller limits;
// each hand owns one independent pointer/axis, combined in one CMD pair.
(() => {
    const axes={v:{pointer:null,sign:0,button:null},w:{pointer:null,sign:0,button:null}};
    let open=false,charts=null,chartParent=null,chartNext=null,ownedFullscreen=false;
    let protectedNodes=[],lastState=-1;
    const get=id=>document.getElementById(id);
    const speed=()=>Math.max(1,Math.min(20,Number(get('drive-speed-limit').value)||5));
    function pair(){return {v:axes.v.sign*speed()/100,w:axes.w.sign*20*Math.PI/180};}
    function showPair(){const p=pair();get('drive-pair-value').textContent=`Tiến/lùi: ${Math.round(p.v*100)} cm/s · Xoay: ${Math.round(p.w*180/Math.PI)}°/s`;}
    function forget(axis){
        const a=axes[axis],button=a.button,pointer=a.pointer;
        a.pointer=null;a.sign=0;a.button=null;
        if(button){button.classList.remove('is-pressed');button.setAttribute('aria-pressed','false');if(button.hasPointerCapture(pointer))button.releasePointerCapture(pointer);}
    }
    function clear(send=true){forget('v');forget('w');showPair();if(send)socket.emit('release');}
    function sendPair(){showPair();if(!socket.emit('drive',pair()))clear(false);}
    function resizeCharts(){
        if(typeof angleChart==='undefined')return;
        for(const c of [angleChart,speedChart,outputChart].filter(Boolean)){
            const wrap=c.canvas.parentElement;c.resize(wrap.clientWidth,wrap.clientHeight);c.update('none');
        }
    }
    function orient(){
        if(!open)return;
        // A local HTTP origin cannot force OS orientation on every browser.
        // The rotated layout gives the same landscape cockpit in that case;
        // browser-transformed pointer coordinates retain normal hit testing.
        const portrait=window.innerHeight>window.innerWidth;
        get('drive-screen').classList.toggle('force-landscape',portrait);
        requestAnimationFrame(()=>{resizeCharts();setTimeout(resizeCharts,80);});
    }
    async function enter(){
        if(open)return;open=true;clear();
        // Viewing is separate from permission: only an explicit Start click
        // grants this dashboard permission to move, including an autonomous Run.
        get('speed-input').value=get('turn-input').value='0';
        get('speed-value').textContent=get('turn-value').textContent='0';
        charts=document.querySelector('#panel-charts .charts-container');chartParent=charts.parentElement;chartNext=charts.nextSibling;
        get('drive-scene-charts').appendChild(charts);
        protectedNodes=[...document.querySelectorAll('body > .header,body > .container,body > .footer,body > .mobile-nav')].map(el=>[el,el.inert]);
        for(const [el]of protectedNodes)el.inert=true;
        get('drive-screen').hidden=false;document.body.classList.add('phone-driving');
        get('drive-scene-speed').value=speed();get('drive-scene-speed-value').value=speed();
        orient();get('drive-exit').focus({preventScroll:true});
        try {
            if(!document.fullscreenElement&&get('drive-screen').requestFullscreen){
                await get('drive-screen').requestFullscreen();
                if(!open){if(document.fullscreenElement===get('drive-screen'))await document.exitFullscreen();return;}
                ownedFullscreen=true;
            }
            if(open&&screen.orientation?.lock)await screen.orientation.lock('landscape');
        }catch(_){/* Landscape CSS fallback remains functional over local HTTP. */}
        if(open)orient();
    }
    async function leave(){
        if(!open)return;open=false;clear();
        if(chartNext&&chartNext.parentElement===chartParent)chartParent.insertBefore(charts,chartNext);else chartParent.appendChild(charts);
        get('drive-screen').hidden=true;document.body.classList.remove('phone-driving');
        for(const [el,inert]of protectedNodes)el.inert=inert;protectedNodes=[];
        try{if(screen.orientation?.unlock)screen.orientation.unlock();}catch(_){}
        const exitFullscreen=ownedFullscreen;ownedFullscreen=false;
        if(exitFullscreen&&document.fullscreenElement)try{await document.exitFullscreen();}catch(_){}
        requestAnimationFrame(resizeCharts);get('btn-open-drive').focus({preventScroll:true});
    }
    function up(event){
        for(const key of ['v','w'])if(axes[key].pointer===event.pointerId){forget(key);if(open)sendPair();}
        for(const a of Object.values(axes))if(a.button)a.button.classList.add('is-pressed');
    }
    document.addEventListener('DOMContentLoaded',()=>{
        document.body.classList.add('phone-direct');
        const button=document.createElement('button');button.id='btn-open-drive';button.className='btn btn-primary btn-full phone-enter-drive';button.type='button';button.textContent='🎮 ĐIỀU KHIỂN XE';button.onclick=enter;
        document.querySelector('.drive-pad').before(button);
        get('drive-exit').onclick=leave;
        get('drive-start').onclick=()=>socket.emit('robotControl',{command:'start'});
        get('drive-stop').onclick=()=>socket.emit('pauseControl');
        for(const b of document.querySelectorAll('#drive-screen [data-axis]')){
            b.disabled=true;b.setAttribute('aria-pressed','false');
            b.addEventListener('contextmenu',e=>e.preventDefault());
            b.addEventListener('pointerdown',e=>{
                const a=axes[b.dataset.axis];
                if(!open||!socket.connected||!socket.controlStarted||robotState.st!==2||a.pointer!==null||Object.values(axes).some(x=>x.pointer===e.pointerId)){if(a.button!==b)b.classList.remove('is-pressed');return;}
                e.preventDefault();a.pointer=e.pointerId;a.sign=Number(b.dataset.sign);a.button=b;b.setPointerCapture(e.pointerId);b.classList.add('is-pressed');b.setAttribute('aria-pressed','true');sendPair();
            });
        }
        for(const name of ['pointerup','pointercancel','lostpointercapture'])document.addEventListener(name,up,true);
        document.addEventListener('balancebot:release',()=>clear(false));
        window.addEventListener('blur',()=>clear());window.addEventListener('pagehide',()=>clear());
        document.addEventListener('visibilitychange',()=>{if(document.hidden)clear();});
        document.addEventListener('keydown',e=>{if(open&&e.key==='Escape'){e.preventDefault();leave();}});
        window.addEventListener('resize',()=>{if(open){clear();orient();}});
        document.addEventListener('fullscreenchange',()=>{if(open&&ownedFullscreen&&!document.fullscreenElement)leave();});
        get('drive-scene-speed').oninput=()=>{
            get('drive-speed-limit').value=get('drive-scene-speed').value;
            get('drive-speed-selected').value=get('drive-scene-speed').value;
            get('drive-scene-speed-value').value=speed();showPair();
        };
        socket.on('serialData',d=>{
            const terminal=get('drive-live-status'),fault=[3,4].includes(d.st);
            terminal.textContent=fault?(d.fault==='FALL'?'XE ĐÃ ĐỔ':d.fault==='SPEED_RAIL'?'QUÁ TỐC':'LỖI: '+(d.fault||'KHÔNG RÕ')):d.st===2?'ĐANG CÂN BẰNG':d.imu_calibrated!==1?'CHỜ HIỆU CHUẨN':d.st===1?'ĐANG BẮT ĐẦU':'ĐANG CHỜ';
            terminal.className='drive-terminal '+(fault?'status-fault':d.st===2?'status-balanced':'status-waiting');
            if(d.st!==2){if(lastState===2)clear();}lastState=d.st;
            for(const b of document.querySelectorAll('#drive-screen [data-axis]'))b.disabled=d.st!==2||!socket.controlStarted;
            get('drive-start').disabled=socket.controlStarted||socket.controlPending||d.imu_calibrated!==1||![0,2].includes(d.st);
        });
        socket.on('controlPermission',()=>{for(const b of document.querySelectorAll('#drive-screen [data-axis]'))b.disabled=robotState.st!==2||!socket.controlStarted;});
        socket.on('disconnect',()=>{clear(false);get('drive-live-status').textContent='MẤT KẾT NỐI';get('drive-live-status').className='drive-terminal status-fault';for(const b of document.querySelectorAll('#drive-screen [data-axis]'))b.disabled=true;});
        setInterval(()=>{if(open&&(axes.v.sign||axes.w.sign))sendPair();},120);
    });
})();
