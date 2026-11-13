let register=false;
const el=id=>document.getElementById(id);
el('toggle').onclick=()=>{register=!register;el('form-title').textContent=register?'Tạo tài khoản':'Đăng nhập';el('submit').textContent=register?'Đăng ký tài khoản':'Đăng nhập';el('toggle').textContent=register?'Đã có tài khoản? Đăng nhập':'Chưa có tài khoản? Đăng ký';el('confirm-row').hidden=!register;el('confirm').required=register;el('password').autocomplete='new-password';el('message').textContent='';el('form-note').textContent=register?'Tên tài khoản 3–32 chữ/số/_. Mật khẩu ít nhất 8 ký tự.':'Đăng nhập để mở giao diện giám sát xe.';};
el('auth-form').onsubmit=async e=>{e.preventDefault();el('message').textContent='';if(register&&el('password').value!==el('confirm').value){el('message').textContent='Mật khẩu nhập lại chưa khớp.';return;}el('submit').disabled=true;try{const r=await fetch('/api/auth/'+(register?'register':'login'),{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({username:el('username').value.trim(),password:el('password').value})});const data=await r.json();if(!r.ok)throw Error(data.error);if(register){el('toggle').click();el('username').value='';el('password').value=el('confirm').value='';el('dashboard-link').hidden=true;el('message').textContent='Đăng ký thành công. Tài khoản đã được lưu. Vui lòng đăng nhập để vào web.';el('password').focus();}else location.assign('/');}catch(error){el('message').textContent=error.message;}finally{el('submit').disabled=false;}};
fetch('/api/auth/me').then(r=>r.json()).then(s=>{if(s.username){el('dashboard-link').hidden=false;el('form-note').textContent='Đã đăng nhập: '+s.username;}}).catch(()=>{});

// Clear restored form values when arriving from logout or browser history.
function clearCredentials(){for(const id of ['username','password','confirm'])el(id).value='';}
window.addEventListener('pageshow',clearCredentials);
window.addEventListener('pagehide',clearCredentials);
clearCredentials();
