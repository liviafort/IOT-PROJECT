// ============================================================================
// CONFIGURAÇÃO
// ============================================================================
const API_BASE = 'https://backsubzero-sandbox.idegra.org.br/mqtt-ui';
const WS_URL = 'https://backsubzero-sandbox.idegra.org.br/mqtt-ws/';

// ============================================================================
// ESTADO DA APLICAÇÃO
// ============================================================================
const state = {
    chartData: {
        labels: [],
        temperatura: [],
        umidade: [],
        luminosidade: []
    },
    stats: {
        temp: { min: Infinity, max: -Infinity },
        humidity: { min: Infinity, max: -Infinity },
        light: { min: Infinity, max: -Infinity }
    }
};

// ============================================================================
// CONEXÃO SOCKET.IO
// ============================================================================
const socket = io(WS_URL, {
    transports: ['websocket', 'polling']
});

socket.on('connect', () => {
    console.log('✅ Socket.IO conectado');
    updateConnectionStatus('Conectado', true);
});

socket.on('disconnect', () => {
    console.log('❌ Socket.IO desconectado');
    updateConnectionStatus('Desconectado', false);
});

socket.on('connect_error', (error) => {
    console.error('❌ Erro de conexão Socket.IO:', error);
    updateConnectionStatus('Erro de conexão', false);
});

socket.on('sensor-data', (data) => {
    console.log('📨 Dados recebidos:', data);
    updateChart(data);
});

// ============================================================================
// GRÁFICO (Chart.js)
// ============================================================================
const ctx = document.getElementById('sensorChart').getContext('2d');
const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: state.chartData.labels,
        datasets: [
            {
                label: 'Temperatura (°C)',
                data: state.chartData.temperatura,
                borderColor: '#ef4444',
                backgroundColor: 'rgba(239, 68, 68, 0.1)',
                tension: 0.4
            },
            {
                label: 'Umidade (%)',
                data: state.chartData.umidade,
                borderColor: '#3b82f6',
                backgroundColor: 'rgba(59, 130, 246, 0.1)',
                tension: 0.4
            },
            {
                label: 'Luminosidade',
                data: state.chartData.luminosidade,
                borderColor: '#f59e0b',
                backgroundColor: 'rgba(245, 158, 11, 0.1)',
                tension: 0.4,
                yAxisID: 'y1'
            }
        ]
    },
    options: {
        responsive: true,
        maintainAspectRatio: true,
        interaction: {
            mode: 'index',
            intersect: false
        },
        plugins: {
            legend: {
                position: 'top'
            }
        },
        scales: {
            y: {
                type: 'linear',
                display: true,
                position: 'left',
                title: {
                    display: true,
                    text: 'Temp (°C) / Umidade (%)'
                }
            },
            y1: {
                type: 'linear',
                display: true,
                position: 'right',
                title: {
                    display: true,
                    text: 'Luminosidade (lux)'
                },
                grid: {
                    drawOnChartArea: false
                }
            }
        }
    }
});

// ============================================================================
// FUNÇÕES DE ATUALIZAÇÃO
// ============================================================================
function updateChart(data) {
    const maxPoints = 20;
    const timestamp = new Date(data.timestamp).toLocaleTimeString('pt-BR', {
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    });

    // Adicionar novo ponto
    state.chartData.labels.push(timestamp);
    state.chartData.temperatura.push(data.temperatura || 0);
    state.chartData.umidade.push(data.umidade || 0);
    state.chartData.luminosidade.push(data.luminosidade || 0);

    // Limitar a 20 pontos
    if (state.chartData.labels.length > maxPoints) {
        state.chartData.labels.shift();
        state.chartData.temperatura.shift();
        state.chartData.umidade.shift();
        state.chartData.luminosidade.shift();
    }

    chart.update();
}


// ============================================================================
// API CALLS
// ============================================================================
async function fetchDevices() {
    try {
        const response = await fetch(`${API_BASE}/api/devices`);
        const result = await response.json();

        if (result.success) {
            displayDevices(result.data);
        }
    } catch (error) {
        console.error('❌ Erro ao buscar dispositivos:', error);
    }
}

async function fetchInitialData() {
    try {
        const response = await fetch(`${API_BASE}/api/sensor-data?limit=20`);
        const result = await response.json();

        if (result.success && result.data.length > 0) {
            // Preencher gráfico com dados históricos (invertido para ordem cronológica)
            result.data.reverse().forEach(data => {
                updateChart(data);
            });
        }
    } catch (error) {
        console.error('❌ Erro ao buscar dados iniciais:', error);
    }
}

function displayDevices(devices) {
    const deviceList = document.getElementById('device-list');

    if (devices.length === 0) {
        deviceList.innerHTML = '<p style="text-align: center; color: #999;">Nenhum dispositivo encontrado</p>';
        return;
    }

    deviceList.innerHTML = devices.map(device => {
        const lastReading = new Date(device.last_reading).toLocaleString('pt-BR');
        return `
            <div class="device-item" data-device="${device.device}">
                <div class="device-icon">📱</div>
                <div class="device-info">
                    <h3>${device.device}</h3>
                    <p>Última leitura: ${lastReading}</p>
                </div>
                <div class="device-badge">${device.total_readings} leituras</div>
            </div>
        `;
    }).join('');

    // Adicionar event listeners para cada dispositivo
    document.querySelectorAll('.device-item').forEach(item => {
        item.addEventListener('click', async () => {
            const deviceId = item.dataset.device;

            // Remover classe active de todos os itens
            document.querySelectorAll('.device-item').forEach(i => i.classList.remove('active'));

            // Adicionar classe active ao item clicado
            item.classList.add('active');

            // Buscar e exibir dados do dispositivo
            await fetchDeviceData(deviceId);
        });
    });
}

async function fetchDeviceData(deviceId) {
    const detailsContainer = document.getElementById('device-details');
    const detailsTitle = document.getElementById('device-details-title');
    const detailsContent = document.getElementById('device-details-content');

    try {
        // Mostrar loading
        detailsContainer.classList.add('show');
        detailsTitle.textContent = `Dados do dispositivo: ${deviceId}`;
        detailsContent.innerHTML = '<p style="text-align: center; color: #999;">Carregando dados...</p>';

        // Buscar dados da API
        const response = await fetch(`${API_BASE}/api/sensor-data?device=${deviceId}`);
        const result = await response.json();

        if (result.success && result.data.length > 0) {
            displayDeviceData(result.data, deviceId);
        } else {
            detailsContent.innerHTML = '<p style="text-align: center; color: #999;">Nenhum dado encontrado para este dispositivo</p>';
        }
    } catch (error) {
        console.error('Erro ao buscar dados do dispositivo:', error);
        detailsContent.innerHTML = '<p style="text-align: center; color: #ef4444;">Erro ao carregar dados</p>';
    }
}

function displayDeviceData(data, deviceId) {
    const detailsContent = document.getElementById('device-details-content');

    // Pegar os últimos 20 registros
    const recentData = data.slice(-20);

    const tableHTML = `
        <table class="data-table">
            <thead>
                <tr>
                    <th>Data/Hora</th>
                    <th>Temperatura (°C)</th>
                    <th>Umidade (%)</th>
                    <th>Luminosidade</th>
                    <th>RSSI (dBm)</th>
                    <th>Uptime (s)</th>
                </tr>
            </thead>
            <tbody>
                ${recentData.map(item => {
                    const timestamp = new Date(item.timestamp).toLocaleString('pt-BR');
                    return `
                        <tr>
                            <td>${timestamp}</td>
                            <td>${item.temperatura != null ? item.temperatura : '-'}</td>
                            <td>${item.umidade != null ? item.umidade : '-'}</td>
                            <td>${item.luminosidade != null ? item.luminosidade : '-'}</td>
                            <td>${item.rssi != null ? item.rssi : '-'}</td>
                            <td>${item.uptime != null ? item.uptime : '-'}</td>
                        </tr>
                    `;
                }).join('')}
            </tbody>
        </table>
        <p style="margin-top: 15px; text-align: center; color: #666; font-size: 0.9em;">
            Mostrando ${recentData.length} registro(s) mais recente(s)
        </p>
    `;

    detailsContent.innerHTML = tableHTML;
}

// ============================================================================
// INICIALIZAÇÃO
// ============================================================================
async function init() {
    console.log('🚀 Inicializando dashboard...');
    await fetchInitialData();
    await fetchDevices();

    // Atualizar lista de dispositivos a cada 30s
    setInterval(fetchDevices, 10000);
}

// Iniciar quando a página carregar
window.addEventListener('DOMContentLoaded', init);
