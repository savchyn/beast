<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-projectshell - Main shell used to display a BSE project
  ### Props:
  - **bse-project** - A *Bse.Project* instance to be displayed and edited.
  - **project** - Read-only, the value of *bse-project*.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .vc-projectshell { }
</style>

<template>
  <div class="vc-projectshell">
    Project Shell. <br />
    <vc-playcontrols> </vc-playcontrols>
    <br />
    {{ message }}
    <br />
    <vc-button @click="show_aboutdialog = true">About</vc-button>
    <vc-button @click="frob (1, $event)">1</vc-button>
    <vc-button @click="frob (2, $event)">2</vc-button>
    <vc-button @click="frob (3, $event)">3</vc-button>
    <vc-aboutdialog v-if="show_aboutdialog" @close="show_aboutdialog = false">
    </vc-aboutdialog>
  </div>
</template>

<script>
module.exports = {
  name: 'vc-projectshell',
  data: function() {
    let d = {
      message: 'Interpolated text',
      show_aboutdialog: false,
    };
    for (const attrname in Electron.app)
      d[attrname] = Electron.app[attrname];
    return d;
  },
  methods: {
    command (cmd) {
      switch (cmd) {
	case 'about-dialog':
	  this.show_aboutdialog = !this.show_aboutdialog;
	  break;
	default:
	  console.log ('SHELLCOMMAND: ' + cmd);
	  break;
      }
    },
    frob (w, e) { App.status (w); },
  },
  computed: {
    project: function () {
      if (!this.bse_project)
	this.bse_project = this['bse-project'];
      if (!this.bse_project)
	this.bse_project = Bse.server.create_project ('Untitled');
      return this.bse_project;
    }
  },
  props: {
    'bse-project': {
      type: Object,
      validator (value) { return value instanceof Bse.Project; },
    },
  },
};
</script>
