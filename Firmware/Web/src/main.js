import { createApp } from 'vue'
import App from './App.vue'
import { Tooltip } from 'bootstrap'

// Import our custom CSS
import './scss/styles.scss'

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
app.config.globalProperties.api_prefix = "/api/v1/"
app.mixin(myMixin)
app.mount('#app')

app.directive('tooltip', {
  mounted(el, binding) {  
      el.setAttribute('data-toggle', 'tooltip')
      
      new Tooltip(el,{
          title: binding.value,
          placement: binding.arg,
          trigger: 'hover'
      })
  }
})