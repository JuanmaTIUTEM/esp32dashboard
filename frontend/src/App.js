import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { Table, Container, Card } from 'react-bootstrap';

const App = () => {
  const [data, setData] = useState([]);

  useEffect(() => {
    fetchData();
    const interval = setInterval(fetchData, 5000);  // Actualización cada 5 segundos
    return () => clearInterval(interval);
  }, []);

  const fetchData = async () => {
    try {
      const response = await axios.get('http://localhost:3001/api/datos');
      setData(response.data);
    } catch (error) {
      console.error('Error al obtener datos:', error);
    }
  };

  return (
    <Container className="my-5">
      <h1 className="text-center">ESP32 Monitor</h1>

      <Card className="p-3 mb-4 shadow-sm">
        <h4>Últimos datos recibidos</h4>

        <Table striped bordered hover responsive>
          <thead>
            <tr>
              <th>ID</th>
              <th>Distancia (cm)</th>
              <th>Temperatura (°C)</th>
              <th>Humedad (%)</th>
              <th>IP</th>
              <th>Fecha</th>
            </tr>
          </thead>
          <tbody>
            {data.map((item) => (
              <tr key={item.id}>
                <td>{item.id}</td>
                <td>{item.distancia.toFixed(2)}</td>
                <td>{item.temperatura.toFixed(2)}</td>
                <td>{item.humedad.toFixed(2)}</td>
                <td>{item.ip}</td>
                <td>{new Date(item.fecha).toLocaleString()}</td>
              </tr>
            ))}
          </tbody>
        </Table>
      </Card>
    </Container>
  );
};

export default App;
