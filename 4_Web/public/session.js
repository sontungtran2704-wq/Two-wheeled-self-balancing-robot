document.getElementById('logout').onclick=async()=>{try{const r=await fetch('/api/auth/logout',{method:'POST'});if(r.ok)location.assign('/welcome');}catch{alert('Chưa đăng xuất được. Kiểm tra kết nối web.');}};
socket.on('connect_error',e=>{if(e.message==='AUTH_REQUIRED')location.assign('/welcome');});
socket.on('disconnect',reason=>{if(reason==='io server disconnect')location.assign('/welcome');});
