const repl = require('repl');
const RpcDuplexStream = require('./rpcDuplexStream').RpcDuplexStream;


class ReplManager {
  constructor(context) {
    this.currentId = 0;
    // id: {server, stream} map
    this.servers = new Map();
    this.context = context
  }

  processRpcMessage(args, callback) {
    const action = args.action;
    const id = args.id;
    const message = args.message;
    console.log('REPL: Processing message', args);
    switch (action) {
      case 'open':
        callback('callback', this._getNewSession());
        return;
      case 'close':
        callback('callback', this._closeSession(id));
        return;
      case 'command':
        this._processMessageForId(id, message, callback);
        return;
      default:
        console.error(`Unknown action ${action}`);
    }
  }

  _getNewSession() {
    console.log('ROV: Getting new session');
    this.currentId += 1;
    const id = this.currentId;

    const newServer = {};
    newServer.stream = new RpcDuplexStream();
    newServer.repl = repl.start({
      prompt: '',
      ignoreUndefined: true,
      input: newServer.stream,
      output: newServer.stream,
    }).on('exit', () => {
      console.warn(`Exit in server id ${id}`);
    });
    if (this.context) {
      Object.defineProperty(newServer.repl.context, 'context', {
        configurable: false,
        enumerable: true,
        value: this.context,
      });
    }
    this.servers.set(id, newServer);
    console.log(`ROV: New Repl session ${id}`);
    return id;
  }
  _closeSession(serverId) {
    console.log(`ROV: Closing session ${serverId}`);
    const replServer = this.servers.get(serverId);
    if (replServer) {
      replServer.repl.close();
      this.servers.delete(serverId);
      console.log(`ROV: Session is closed ${serverId}`);
      return true;
    }
    return false;
  }
  _processMessageForId(serverId, message, callback) {
    console.log(`ROV: Processing message for session: ${serverId}, message: ${message}`);
    const replServer = this.servers.get(serverId);
    if (replServer) {
      replServer.stream.onData(message, callback);
    }
  }
}

exports.ReplManager = ReplManager;

