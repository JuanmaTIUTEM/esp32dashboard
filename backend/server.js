const express = require('express');
const mysql = require('mysql2');
const bodyParser = require('body-parser');
const cors = require('cors');

const app = express();
const port = 3001;

app.use(cors());
app.use(bodyParser.json());

// Conexión a la base de datos
const db = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: '',  
    database: 'esp32db'
});

db.connect((err) => {
    if (err) {
        console.error('Error de conexión a MySQL:', err);
    } else {
        console.log('Conectado a MySQL');
    }
});

// Ruta para recibir datos desde el ESP32
app.post('/api/datos', (req, res) => {
    const { distancia, temperatura, humedad, ip } = req.body;

    const sql = `INSERT INTO sensores (distancia, temperatura, humedad, ip) VALUES (?, ?, ?, ?)`;

    db.query(sql, [distancia, temperatura, humedad, ip], (err, result) => {
        if (err) {
            console.error('Error al insertar datos:', err);
            res.status(500).send('Error al insertar datos');
        } else {
            res.status(201).send('Datos almacenados correctamente');
        }
    });
});

// Ruta para obtener los datos desde la base de datos
app.get('/api/datos', (req, res) => {
    const sql = `SELECT * FROM sensores ORDER BY fecha DESC LIMIT 10`;

    db.query(sql, (err, results) => {
        if (err) {
            console.error('Error al obtener datos:', err);
            res.status(500).send('Error al obtener datos');
        } else {
            res.json(results);
        }
    });
});

app.listen(port, () => {
    console.log(`Servidor corriendo en http://localhost:${port}`);
});
