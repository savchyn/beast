module.exports = {
  "env": {
    "browser": true,
    "es6": true,
    "jquery": true,
    "node": true
  },
  "globals": {
    "Electron": false,
    "Mithril": false,
    "Bse": false,
    "App": false,
    "module": true /* allow mods */
  },
  "rules": {
    "no-unused-vars": [ "warn", { "argsIgnorePattern": "^_.*", "varsIgnorePattern": "^_.*" } ],
    "no-unreachable": [ "warn" ],
    "semi": [ "error", "always" ],
    "no-extra-semi": [ "warn" ],
    "no-console": [ "off" ],
    "no-constant-condition": [ "warn" ],
    "indent": [ "off", 2 ],
    "linebreak-style": [ "error", "unix" ],
    "no-mixed-spaces-and-tabs": [ "off" ],
    "quotes": [ "off", "single" ]
  },
  "plugins": [ "html" ],
  "extends": "eslint:recommended"
};
