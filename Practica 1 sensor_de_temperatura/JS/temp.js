function cambiarTemperatura() {
    // Genera una temperatura aleatoria entre 20 y 40 grados Celsius
    const nuevaTemp = Math.floor(Math.random() * 21) + 20;
    document.getElementById('temp').innerText = nuevaTemp;
}
setInterval(cambiarTemperatura, 2000); 
