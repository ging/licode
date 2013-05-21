var config = {}

config.rabbit = {};
config.nuve = {};
config.erizoController = {};
config.cloudProvider = {};

config.rabbit.host = 'localhost';
config.rabbit.port = 5672;

config.nuve.dataBaseURL = "localhost/nuvedb";
config.nuve.superserviceID = '_auto_generated_ID_';
config.nuve.superserviceKey = '_auto_generated_KEY_';
config.nuve.testErizoController = 'localhost:8080';

//Use undefined to run clients without Stun 
config.erizoController.stunServerUrl = 'stun:stun.l.google.com:19302';
config.erizoController.warning_n_rooms = 15;
config.erizoController.limit_n_rooms = 20;
config.erizoController.interval_time_keepAlive = 1000;

config.cloudProvider.name = '';

module.exports = config;
