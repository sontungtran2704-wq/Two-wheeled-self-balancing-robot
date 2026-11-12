'use strict';

// Local files cannot use the server's relative Socket.IO and serial API URLs.
// Opening index.html remains a convenient entry point to the running dashboard.
if (window.location.protocol === 'file:') {
    window.location.replace('http://localhost:3000/');
}
