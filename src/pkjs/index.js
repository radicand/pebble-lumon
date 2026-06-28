var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

var SETTINGS_KEY = 'lumontime-settings';

function loadSettings() {
  try {
    var raw = localStorage.getItem(SETTINGS_KEY);
    return raw ? JSON.parse(raw) : {};
  } catch (error) {
    console.log('Failed to parse settings:', error);
    return {};
  }
}

function saveSettings(settings) {
  localStorage.setItem(SETTINGS_KEY, JSON.stringify(settings));
}

function sendSettings(settings) {
  if (settings.ANIM_MODE === undefined) {
    return;
  }

  Pebble.sendAppMessage({ ANIM_MODE: settings.ANIM_MODE },
    function() {
      console.log('Sent ANIM_MODE to watch');
    },
    function(error) {
      console.log('AppMessage failed:', error);
    }
  );
}

function syncClayFromSettings(settings) {
  if (settings.ANIM_MODE !== undefined) {
    clay.setSettings({ ANIM_MODE: settings.ANIM_MODE });
  }
}

Pebble.addEventListener('ready', function() {
  var settings = loadSettings();
  syncClayFromSettings(settings);
  sendSettings(settings);
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(event) {
  if (!event || !event.response) {
    return;
  }

  try {
    var message = clay.getSettings(event.response);
    if (message.ANIM_MODE === undefined) {
      return;
    }

    var settings = { ANIM_MODE: message.ANIM_MODE };
    saveSettings(settings);
    sendSettings(settings);
  } catch (error) {
    console.log('Error processing config:', error);
  }
});
