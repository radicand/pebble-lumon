var SETTINGS_KEY = 'lumontime-settings';

function sendSettings(settings) {
  if (settings.ANIM_MODE === undefined) {
    return;
  }
  Pebble.sendAppMessage({ ANIM_MODE: settings.ANIM_MODE });
}

Pebble.addEventListener('ready', function() {
  var settings = JSON.parse(localStorage.getItem(SETTINGS_KEY) || '{}');
  sendSettings(settings);
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL('file://config.html');
});
