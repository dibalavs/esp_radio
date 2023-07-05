import { createApp } from 'vue'
import App from './App.vue'

// NEW
if (process.env.NODE_ENV === 'development') {
    const { worker } = require('./mocks/browser')
    worker.start()
}

const myMixin = {
    methods: {
      alert(title, message) {
        this.$root.alert_items.push({title:title, message:message})
      }
    }
  }

const app = createApp(App)
app.mixin(myMixin)
app.mount('#app')
