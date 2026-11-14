'use strict';
const qrcode = require('./public/qrcode.min.js');
const url = process.argv[2] || 'http://127.0.0.1:3000';
try {
  const qr = qrcode(0, 'M');
  qr.addData(url);
  qr.make();
  const count = qr.getModuleCount();
  console.log('\n  ┌' + '─'.repeat(count + 4) + '┐');
  // Top quiet zone
  console.log('  │  ' + '█'.repeat(count) + '  │');
  for (let r = 0; r < count; r += 2) {
    let line = '';
    for (let c = 0; c < count; c++) {
      const top = qr.isDark(r, c);
      const bot = (r + 1 < count) ? qr.isDark(r + 1, c) : false;
      if (top && bot) line += ' ';
      else if (top && !bot) line += '▄';
      else if (!top && bot) line += '▀';
      else line += '█';
    }
    console.log('  │  ' + line + '  │');
  }
  // Bottom quiet zone
  console.log('  │  ' + '█'.repeat(count) + '  │');
  console.log('  └' + '─'.repeat(count + 4) + '┘\n');
} catch (e) {
  // If QR fails to render in terminal, fallback silently
}

