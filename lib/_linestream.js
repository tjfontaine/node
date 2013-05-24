var stream = require('stream');
var util = require('util');

var LineStream = module.exports = function(opts) {
  if (!(this instanceof LineStream)) return new LineStream(opts);
  stream.Transform.call(this, opts);
  this._buff = '';
};
util.inherits(LineStream, stream.Transform);

LineStream.prototype._transform = function(chunk, encoding, done) {
  var data = this._buff + chunk.toString('utf8');
  var lines = data.split(/\r?\n|\r(?!\n)/);

  if (!data.match(/(\r?\n|\r(?!\n))$/))
    this._buff = lines.pop();
  else
    this._buff = '';

  var self = this;

  lines.forEach(function (line) {
    if (line.trim())
      self.emit('line', line.replace('\r', ''));
  });

  this.push(chunk);
  done();
};

LineStream.prototype._flush = function(done) {
  if (this._buff) this.emit('line', this._buff);
  done();
};
