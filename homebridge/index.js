let Service, Characteristic;

/** This is still very much a massive WIP.
 *  At present it's a slightly hacked together 
 *  sample provided for basic on/off functionality.
 */

module.exports = function (homebridge) {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  homebridge.registerAccessory("homebridge-lego-train", "LegoTrain", legoTrain);
};

const request = require('request');
const url = require('url');

function legoTrain(log, config) {
  this.log = log;
  this.trainurl = url.parse(config['accessUrl']);
}

legoTrain.prototype = {
    getServices: function () {
        let informationService = new Service.AccessoryInformation();
        informationService
        .setCharacteristic(Characteristic.Manufacturer, "LEGO ")
        .setCharacteristic(Characteristic.Model, "Hogwarts Express - Wemos D1 mini")
        .setCharacteristic(Characteristic.SerialNumber, "127-888-999");

        let switchService = new Service.Switch("LegoTrainSwitch");
        switchService
        .getCharacteristic(Characteristic.On)
        .on('get', this.getSwitchOnCharacteristic.bind(this))
        .on('set', this.setSwitchOnCharacteristic.bind(this));

        this.informationService = informationService;
        this.switchService = switchService;
        return [informationService, switchService];
    },
      getSwitchOnCharacteristic: function (next) {
    const me = this;
    request({
      url: me.trainurl.href+"status",
      method: 'GET',
    },
    function (error, response, body) {
      if (error) {
        me.log('STATUS: ' + JSON.stringify(response));
        me.log(error.message);
        return next(error);
      }
      me.log('repsonse: '+JSON.stringify(response))
      return next(null, response.on);
    });
  },

  setSwitchOnCharacteristic: function (on, next) {
    const me = this;
    me.log(JSON.stringify(me.trainurl));
    //This is slightly (read very) munted until I refactor some of the server code on the wemos.
    if(on){
        //true - start
        me.log(me.trainurl.href+'go');
        request({
            url: me.trainurl.href+'go?speed=70',
            method: 'POST',
            headers: {'Content-type': 'application/json'}
          },
          function (error, response) {
            if (error) {
              me.log('STATUS: ' + JSON.stringify(response));
              me.log(error.message);
              return next(error);
            }
            return next();
          });
    }else{
        //stop - go home
        me.log(me.trainurl.href+'gohome');
        request({
            url: me.trainurl.href+'gohome',
            method: 'GET',
            headers: {'Content-type': 'application/json'}
          },
          function (error, response) {
            if (error) {
              me.log('STATUS: ' + JSON.stringify(response));
              me.log(error.message);
              return next(error);
            }
            return next();
          });
    } 
  }
};



