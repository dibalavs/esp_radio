<template>
  <div class="container g-3">
    <div class="btn-toolbar" role="toolbar">
      <div class="btn-group me-2" role="group">
        <div class="btn btn-primary">
          <label for="ipradio_import">Import</label>
          <input type="file" name="ipradio_import" id="ipradio_import" @change="onFileUploaded" accept=".txt, .m3u" style="opacity:0" hidden="true"/>
        </div>
        <button type="button" class="btn btn-primary" @click="onExport()">Export</button>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: "IpRadioToolbar",
  data() {
    return {
      test:false
    };
  },
  methods: {
    async onFileUploaded(event) {
      const file = event.target.files.item(0)
      console.log(file)
      const text = await file.text();
      fetch(this.api_prefix + "ipradio_import", {
        method: "POST",
        headers: new Headers({
          'Content-Type': file.name.endsWith('.txt') ? 'application/json' : 'application/mpegurl'
        }),
        body: text
      })
    },
    onExport() {
      fetch(this.api_prefix + "ipradio_export")
        .then( res => res.blob() )
        .then(blob => {
            var url = window.URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url;
            a.download = "ipradio.txt";
            document.body.appendChild(a);
            a.click();
            a.remove();
      });
    }
  },
};
</script>