'use strict'//使用严格的javascript语法

const express = require('express')
const app = express()
app.use(express.static('./'))  // 注意相对路径不能写错了
app.listen(8000,()=>{
    console.log('Server is running at http://127.0.0.1:8000')
})