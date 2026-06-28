var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

var SETTINGS_KEY = 'lumontime-settings';

function normalizeAnimMode(value) {
  if (value === undefined || value === null) {
    return undefined;
  }

  var mode = parseInt(value, 10);
  if (mode !== 0 && mode !== 1 && mode !== 2) {
    return undefined;
  }

  return mode;
}

function settingsFromClayResponse(response) {
  var raw = clay.getSettings(response, false);
  var value = raw.ANIM_MODE;

  if (value && typeof value === 'object' && value.value !== undefined) {
    value = value.value;
  }

  return normalizeAnimMode(value);
}

function loadSettings() {
  try {
    var raw = localStorage.getItem(SETTINGS_KEY);
    var settings = raw ? JSON.parse(raw) : {};

    if (settings.ANIM_MODE === undefined) {
      var clayRaw = localStorage.getItem('clay-settings');
      if (clayRaw) {
        var claySettings = JSON.parse(clayRaw);
        var animMode = normalizeAnimMode(claySettings.ANIM_MODE);
        if (animMode !== undefined) {
          settings.ANIM_MODE = animMode;
        }
      }
    }

    return settings;
  } catch (error) {
    console.log('Failed to parse settings:', error);
    return {};
  }
}

function saveSettings(settings) {
  localStorage.setItem(SETTINGS_KEY, JSON.stringify(settings));
}

function sendSettings(settings) {
  var animMode = normalizeAnimMode(settings.ANIM_MODE);
  if (animMode === undefined) {
    return;
  }

  Pebble.sendAppMessage({ ANIM_MODE: animMode },
    function() {
      console.log('Sent ANIM_MODE to watch');
    },
    function(error) {
      console.log('AppMessage failed:', error);
    }
  );
}

function syncClayFromSettings(settings) {
  var animMode = normalizeAnimMode(settings.ANIM_MODE);
  if (animMode !== undefined) {
    clay.setSettings({ ANIM_MODE: String(animMode) });
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
    var animMode = settingsFromClayResponse(event.response);
    if (animMode === undefined) {
      return;
    }

    var settings = { ANIM_MODE: animMode };
    saveSettings(settings);
    sendSettings(settings);
  } catch (error) {
    console.log('Error processing config:', error);
  }
});
