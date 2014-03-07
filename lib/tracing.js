// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

var EventEmitter = require('events');
var v8binding, process;

// This needs to be loaded early, and before the "process" object is made
// global. So allow src/node.js to pass the process object in during
// initialization.
exports._nodeInitialization = function nodeInitialization(pobj) {
  process = pobj;
  v8binding = process.binding('v8');

  // Finish setting up the v8 Object.
  v8.getHeapStatistics = v8binding.getHeapStatistics;

  // Part of the AsyncListener setup to share objects/callbacks with the
  // native layer.
  process._setupAsyncListener(asyncFlags, asyncHandler);

  // Do a little housekeeping.
  delete exports._nodeInitialization;
};


// v8

var v8 = exports.v8 = new EventEmitter();


function emitGC(before, after) {
  v8.emit('gc', before, after);
}


v8.on('newListener', function(name) {
  if (name === 'gc' && EventEmitter.listenerCount(this, name) === 0) {
    v8binding.startGarbageCollectionTracking(emitGC);
  }
});


v8.on('removeListener', function(name) {
  if (name === 'gc' && EventEmitter.listenerCount(this, name) === 0) {
    v8binding.stopGarbageCollectionTracking();
  }
});


// AsyncListener

// Stateful flags shared with Environment for quick JS/C++
// communication.
var asyncFlags = {};

// To prevent infinite recursion when an error handler also throws
// flag when an error is currenly being handled.
var inErrorTick = false;

// Where listeners are stored
var asyncListeners = {
  counts: {}
};

// Needs to be the same as src/env.h
var kActiveContextType = 0;
var kWatchedProviders = 1;
var kWatchedEvents = 2;

// Flags to determine what async listeners are available.
var ASYNC_EVENTS = {
  CREATE: 1 << 0,
  BEFORE: 1 << 1,
  AFTER: 1 << 2,
  ERROR: 1 << 3
};


for (var e in ASYNC_EVENTS)
  ASYNC_EVENTS[ASYNC_EVENTS[e]] = e;


exports.ASYNC_EVENTS = ASYNC_EVENTS;


var ASYNC_PROVIDERS = {
  NONE: 0,
  CRYPTO: 1 << 0,
  FSEVENT: 1 << 1,
  FS: 1 << 2,
  GETADDRINFO: 1 << 3,
  PIPE: 1 << 4,
  PROCESS: 1 << 5,
  QUERY: 1 << 6,
  SHUTDOWN: 1 << 7,
  SIGNAL: 1 << 8,
  STATWATCHER: 1 << 9,
  TCP: 1 << 10,
  TIMER: 1 << 11,
  TLS: 1 << 12,
  TTY: 1 << 13,
  UDP: 1 << 14,
  ZLIB: 1 << 15
};

var ASYNC_PROVIDERS_MAP = {};

for (var p in ASYNC_PROVIDERS) {
  ASYNC_PROVIDERS_MAP[p] = ASYNC_PROVIDERS[p];
  ASYNC_PROVIDERS_MAP[ASYNC_PROVIDERS[p]] = p;
}

exports.ASYNC_PROVIDERS = ASYNC_PROVIDERS_MAP;

// _errorHandler is scoped so it's also accessible by _fatalException.
exports._errorHandler = errorHandler;


// Handle errors that are thrown while in the context of an
// AsyncListener. If an error is thrown from an AsyncListener
// callback error handlers will be called once more to report
// the error, then the application will die forcefully.
function errorHandler(er) {
  if (inErrorTick)
    return false;

  var handled = false;
  var i, queueItem, threw;

  var provider = asyncListeners[asyncFlags[kActiveContextType]];

  if (!provider)
    return false;

  var asyncQueue = provider.error;

  if (!asyncQueue || !asyncQueue.length)
    return false;

  inErrorTick = true;

  for (i = 0; i < asyncQueue.length; i++) {
    queueItem = asyncQueue[i];
    try {
      threw = true;
      handled = queueItem.error(er) || handled;
      threw = false;
    } finally {
      // If the error callback thew then die quickly. Only allow the
      // exit events to be processed.
      // XXX why would we want the exit handlers to fire instead of dying hard?
      if (threw) {
        process._exiting = true;
        process.emit('exit', 1);
      }
    }
  }

  inErrorTick = false;

  return handled;
}


function asyncHandler(providerMask, evMask, handle) {
  var provider = asyncListeners[ASYNC_PROVIDERS_MAP[providerMask]];

  if (!provider)
    return;

  var ev;

  switch(evMask) {
    case ASYNC_EVENTS.CREATE:
      ev = 'create';
      break;
    case ASYNC_EVENTS.BEFORE:
      ev = 'before';
      break;
    case ASYNC_EVENTS.AFTER:
      ev = 'after';
      break;
    default:
      return;
      break;
  }

  var queue = provider[ev];

  if (!queue || !queue.length)
    return;

  for (var i = 0; i < queue.length; i++) {
    var item = queue[i];
    item(uid, handle);
  }
}
exports._asyncHandler = asyncHandler;


// for every provider type that matches the bitmask add the function to the
// callback queue, this is slightly inefficient for for storage and for time
// during adding and removing, but makes dispatch and accounting pretty
// straight forward
function manageListeners(provider, ev, cb, add) {
  for (var p in ASYNC_PROVIDERS) {
    var val = ASYNC_PROVIDERS[p];

    if (!val || (provider & val) !== val)
      continue

    var events = asyncListeners[p] = asyncListeners[p] || {
      create: [],
      before: [],
      after: [],
      error: []
    };

    var evMask = 0;

    switch(ev) {
      case 'create':
        evMask = ASYNC_EVENTS.CREATE;
        break;
      case 'before':
        evMask = ASYNC_EVENTS.BEFORE;
        break;
      case 'after':
        evMask = ASYNC_EVENTS.AFTER;
        break;
      case 'error':
        evMask = ASYNC_EVENTS.ERROR;
        break;
      default:
        throw new Error(ev + ' is not a valid async event type');
        break;
    }

    var queue = events[ev];

    var counts = asyncListeners.counts;

    if (add) {
      if (queue.indexOf(cb) !== -1)
        return;

      queue.push(cb);

      asyncFlags[kWatchedProviders] |= val;
      asyncFlags[kWatchedEvents] |= evMask;

      counts[p] = ++counts[p] || 1;
      counts[ev] = ++counts[ev] || 1;
    } else {
      var idx = queue.indexOf(cb);

      if (idx === -1)
        return;

      queue.splice(idx, 1);

      var providerCount = --counts[p];

      if (providerCount === 0)
        asyncFlags[kWatchedProviders] ^= p;

      assert(providerCount >= 0, 'There should not be less than 0 providers');

      var eventCount = --counts[ev];

      if (eventCount === 0)
        asyncFlags[kWatchedEvents] ^= evMask;

      assert(eventCount >= 0, 'There should not be less than 0 events');
    }
  }
}


exports.onAsync = function onAsync(provider, ev, cb) {
  manageListeners(provider, ev, cb, true);
};


exports.removeAsync = function removeAsync(provider, ev, cb) {
  manageListeners(provider, ev, cb, false);
};
