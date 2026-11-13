'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const {promisify}=require('node:util');
const scrypt=promisify(crypto.scrypt);
function createAuth(file) {
  const sessions=new Map(), attempts=new Map();
  let busy=false;
  function session(req) {
    const token=(req.headers.cookie||'').split(';').map(x=>x.trim()).find(x=>x.startsWith('robot_session='))?.slice(14);
    const entry=sessions.get(token);
    if(!entry || entry.expires<Date.now()){sessions.delete(token);return null;}
    return {token,...entry};
  }
  function users(){try{return JSON.parse(fs.readFileSync(file,'utf8'));}catch(e){if(e.code==='ENOENT')return [];throw e;}}
  function mount(app,logout) {
    app.get('/api/auth/me',(req,res)=>{const s=session(req);res.set('Cache-Control','no-store');res.json({username:s?.username||null});});
    for(const action of ['login','register'])app.post('/api/auth/'+action,async(req,res)=>{
      res.set('Cache-Control','no-store');
      const ip=req.socket.remoteAddress, now=Date.now();
      const history=(attempts.get(ip)||[]).filter(t=>now-t<60000);
      if(history.length>=20)return res.status(429).json({error:'Thử quá nhiều lần. Vui lòng đợi một phút.'});
      history.push(now);attempts.set(ip,history);
      const {username,password}=req.body||{};
      if(typeof username!=='string'||! /^[A-Za-z0-9_]{3,32}$/.test(username)||typeof password!=='string'||password.length<8||password.length>128)
        return res.status(400).json({error:'Tên tài khoản 3–32 ký tự (chữ, số, _); mật khẩu 8–128 ký tự.'});
      if(busy)return res.status(503).json({error:'Đang xử lý, vui lòng thử lại.'});
      busy=true;
      try {
        const all=users(), name=username.toLowerCase();let user=all.find(x=>x.username===name);
        if(action==='register') {
          if(user)return res.status(409).json({error:'Tên tài khoản đã tồn tại.'});
          const salt=crypto.randomBytes(16).toString('hex');
          user={username:name,salt,hash:(await scrypt(password,salt,64)).toString('hex')};
          fs.mkdirSync(path.dirname(file),{recursive:true});
          fs.writeFileSync(file+'.tmp',JSON.stringify([...all,user],null,2),{mode:0o600});fs.renameSync(file+'.tmp',file);
          const old=session(req);if(old){sessions.delete(old.token);logout(old.token);}
          res.setHeader('Set-Cookie','robot_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0');
          return res.status(201).json({username:name,registered:true});
        } else {
          const hash=await scrypt(password,user?.salt||'invalid-user',64);
          if(!user||!crypto.timingSafeEqual(hash,Buffer.from(user.hash,'hex')))return res.status(401).json({error:'Sai tài khoản hoặc mật khẩu.'});
        }
        const old=session(req);if(old){sessions.delete(old.token);logout(old.token);}
        const token=crypto.randomBytes(32).toString('hex');
        sessions.set(token,{username:name,expires:now+8*3600000});
        res.setHeader('Set-Cookie',`robot_session=${token}; HttpOnly; SameSite=Strict; Path=/; Max-Age=28800`);
        res.json({username:name});
      }catch(e){console.error('Account storage error:',e.code||e.name);res.status(500).json({error:'Không thể đọc/lưu tài khoản trên máy chủ.'});}
      finally{busy=false;}
    });
    app.post('/api/auth/logout',(req,res)=>{const s=session(req);if(s){sessions.delete(s.token);logout(s.token);}res.setHeader('Set-Cookie','robot_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0');res.json({success:true});});
  }
  return {session,mount};
}
module.exports={createAuth};
