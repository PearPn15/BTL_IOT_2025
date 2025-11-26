const BASE = "http://192.168.1.39"; 
const lights = [
  "Bàn Giáo Viên (Trái)", "Bàn Giáo Viên (Phải)",
  "Giữa Lớp (Trái)", "Giữa Lớp (Phải)",
  "Cuối Lớp (Trái)", "Cuối Lớp (Phải)"
];

let luxChart;
let currentAutoMode = null;

function init() {
  const container = document.getElementById("gridContainer");
  lights.forEach((name, id) => {
    container.innerHTML += `
      <div class="lamp-item off" id="lamp-${id}" onclick="toggleLight(${id})">
        <div class="lamp-info">
          <span class="lamp-name">${name}</span>
          <span class="lamp-state-text" id="txt-${id}">Đang tắt</span>
        </div>
        <i class="fas fa-lightbulb bulb-icon"></i>
      </div>
    `;
  });

  const ctx = document.getElementById('luxChart').getContext('2d');
  luxChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: [],
      datasets: [{
        label: 'Độ sáng (Lux)',
        data: [],
        borderColor: '#fbbc04',
        backgroundColor: 'rgba(251,188,4,0.15)',
        borderWidth: 2,
        pointRadius: 2,
        tension: 0.3,
        fill: true
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: { 
          title: { display: true, text: "Thời gian" },
          ticks: { maxTicksLimit: 5 }
        },
        y: {
          title: { display: true, text: "Lux" },
          beginAtZero: true
        }
      },
      plugins: { legend: { display: true } }
    }
  });

  setInterval(updateStatus, 1000);
}

function updateStatus() {
  fetch(`${BASE}/status`)
    .then(res => res.json())
    .then(data => {

      // --- cập nhật giá trị lux ---
      const luxValue = Number(data.lux);
      document.getElementById("luxValue").innerText = luxValue.toFixed(0);
      addDataToChart(luxValue);

      // --- cập nhật chế độ ---
      if (currentAutoMode !== data.auto) {
        currentAutoMode = data.auto;
        document.getElementById(data.auto ? "mode-auto" : "mode-manual").checked = true;
        updateUIOverlay(data.auto);
      }

      // --- cập nhật trạng thái từng đèn ---
      data.relays.forEach((state, id) => {
        const lamp = document.getElementById(`lamp-${id}`);
        const txt = document.getElementById(`txt-${id}`);
        if (state == 1) {
          lamp.classList.add("on"); 
          lamp.classList.remove("off");
          txt.innerText = "ĐANG BẬT";
        } else {
          lamp.classList.remove("on");
          lamp.classList.add("off");
          txt.innerText = "Đang tắt";
        }
      });

      // --- trạng thái kết nối ---
      const st = document.getElementById("status-indicator");
      st.innerText = "Đã đồng bộ";
      st.style.color = "green";

    })
    .catch(err => {
      document.getElementById("status-indicator").innerText = "Mất kết nối";
      document.getElementById("status-indicator").style.color = "red";
      console.log("ERR:", err);
    });
}

function toggleLight(id) {
  if (currentAutoMode) return; // khóa khi auto
  const lamp = document.getElementById(`lamp-${id}`);
  const state = lamp.classList.contains("on") ? 0 : 1;
  fetch(`${BASE}/toggle?id=${id}&state=${state}`);
}

function changeMode(val) {
  fetch(`${BASE}/setMode?auto=${val}`)
    .then(() => {
      currentAutoMode = (val == 1);
      updateUIOverlay(currentAutoMode);
    });
}

function updateUIOverlay(isAuto) {
  const overlay = document.getElementById("lockOverlay");
  if (isAuto) overlay.classList.add("active");
  else overlay.classList.remove("active");
}

function addDataToChart(val) {
  const now = new Date();
  const label = now.toLocaleTimeString("vi-VN", { hour12: false });

  luxChart.data.labels.push(label);
  luxChart.data.datasets[0].data.push(val);

  if (luxChart.data.labels.length > 20) {
    luxChart.data.labels.shift();
    luxChart.data.datasets[0].data.shift();
  }

  luxChart.update();
}

init();
