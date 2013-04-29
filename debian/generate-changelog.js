var fs = require('fs');

fs.readFile('ChangeLog', 'utf8', function (err, data) {
  var changelog = fs.createWriteStream('debian/changelog');

  var inVer;
  
  data.split(/\n/).forEach(function (line) {
    if (!line.trim()) return;

    var version, date, d, t;

    version = line.match(/^(\d+\.\d+\.\d+), Version (\d+\.\d+\.\d+(-nightly-\d+)?)/);

    if (version) {
      if (inVer) {
        var date = inVer[1].split('.');
        date = new Date(date[0], date[1], date[2]);
        d = date.toString().split(' ');

        d[0] += ',';

        t = d[1];
        d[1] = d[2];
        d[2] = t;

        d[5] = d[5].replace('GMT', '');

        changelog.write(' -- Nodejs Jenkins <node+jenkins@joyent.com>  ');
        changelog.write(d.slice(0, 6).join(' ') + '\n');
      }
      changelog.write('node (' + version[2] + ') unstable; urgency=low\n');
      inVer = version;
    } else {
      changelog.write('  ' + line + '\n');
    }
  });
});
