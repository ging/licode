/* global window */

import io from '../lib/socket.io';
import Logger from './utils/Logger';

import { EventDispatcher, LicodeEvent } from './Events';

const SocketEvent = (type, specInput) => {
  const that = LicodeEvent({ type });
  that.args = specInput.args;
  return that;
};

/*
 * Class Stream represents a local or a remote Stream in the Room. It will handle the WebRTC
 * stream and identify the stream and where it should be drawn.
 */
const Socket = (newIo) => {
  const that = EventDispatcher();
  const defaultCallback = () => {};

  that.CONNECTED = Symbol('connected');
  that.DISCONNECTED = Symbol('disconnected');

  that.state = that.DISCONNECTED;
  that.IO = newIo === undefined ? io : newIo;

  let socket;

  const emit = (type, ...args) => {
    that.emit(SocketEvent(type, { args }));
  };

  that.connect = (token, callback = defaultCallback, error = defaultCallback) => {
    const options = {
      reconnect: false,
      reconnectionAttempts: 3,
      secure: token.secure,
      forceNew: true,
      transports: ['websocket'],
      rejectUnauthorized: false,
    };
    let transport = token.secure ? 'wss://' : 'ws://';
    let host = token.host;
    let id;
    if (window.window.location.search.indexOf('no-secure=1') !== -1) {
      transport = 'ws://';
      host = 'localhost:8080';
    }
    socket = that.IO.connect(transport + host, options);

    socket.on('onAddStream', emit.bind(that, 'onAddStream'));

    socket.on('signaling_message_erizo', emit.bind(that, 'signaling_message_erizo'));
    socket.on('signaling_message_peer', emit.bind(that, 'signaling_message_peer'));
    socket.on('publish_me', emit.bind(that, 'publish_me'));
    socket.on('onBandwidthAlert', emit.bind(that, 'onBandwidthAlert'));

    // We receive an event of new data in one of the streams
    socket.on('onDataStream', emit.bind(that, 'onDataStream'));

    // We receive an event of new data in one of the streams
    socket.on('onUpdateAttributeStream', emit.bind(that, 'onUpdateAttributeStream'));

    // We receive an event of a stream removed from the room
    socket.on('onRemoveStream', emit.bind(that, 'onRemoveStream'));

    // The socket has disconnected
    socket.on('disconnect', (reason) => {
      Logger.error('Disconnect!', reason);
      // emit.bind(that, 'disconnect');
    });

    socket.on('connection_failed', () => {
      Logger.error('Connection failed!');
      emit.bind(that, 'connection_failed');
    });
    socket.on('error', (err) => {
      Logger.error('Socket error!!', err);
      emit.bind(that, 'error');
    });
    socket.on('connect_error', (err) => {
      Logger.error('Connect Error!!', err);
    });

    socket.on('connect_timeout', (err) => {
      Logger.error('Connect Timeout!!', err);
    });

    socket.on('reconnecting', (attemptNumber) => {
      Logger.error('Reconnecting!!', attemptNumber);
    });

    socket.on('reconnect', (attemptNumber) => {
      Logger.error('Reconnect!!', attemptNumber);
      socket.emit('reconnected', id);
    });

    socket.on('reconnect_attempt', (attemptNumber) => {
      Logger.error('Reconnect Attempt!!', attemptNumber);
    });

    socket.on('reconnect_error', (err) => {
      Logger.error('Reconnect Error!!', err);
    });

    socket.on('reconnect_failed', () => {
      Logger.error('Reconnect Failed!!');
      emit.bind(that, 'disconnect');
    });

    // First message with the token
    that.sendMessage('token', token, (response) => {
      that.state = that.CONNECTED;
      id = response.socketId;
      callback(response);
    }, error);
  };

  that.disconnect = () => {
    that.state = that.DISCONNECTED;
    socket.disconnect();
  };

  // Function to send a message to the server using socket.io
  that.sendMessage = (type, msg, callback = defaultCallback, error = defaultCallback) => {
    if (that.state === that.DISCONNECTED && type !== 'token') {
      Logger.error('Trying to send a message over a disconnected Socket');
      return;
    }
    socket.emit(type, msg, (respType, resp) => {
      if (respType === 'success') {
        callback(resp);
      } else if (respType === 'error') {
        error(resp);
      } else {
        callback(respType, resp);
      }
    });
  };

  // It sends a SDP message to the server using socket.io
  that.sendSDP = (type, options, sdp, callback = defaultCallback) => {
    if (that.state === that.DISCONNECTED) {
      Logger.error('Trying to send a message over a disconnected Socket');
      return;
    }
    socket.emit(type, options, sdp, (response, respCallback) => {
      callback(response, respCallback);
    });
  };
  return that;
};

export { SocketEvent, Socket };
