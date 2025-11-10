const GEMINI_API_KEY = "AIzaSyAP4huhhJXJ_0kSzbOrLcXQYStJmwOgztA";
const MODEL = "gemini-2.0-flash"; 

const promptInput = document.getElementById("prompt");
const generateBtn = document.getElementById("generateBtn");
const regenerateBtn = document.getElementById("regenerateBtn");
const copyBtn = document.getElementById("copyBtn");
const loadingState = document.getElementById("loadingState");
const resultsContainer = document.getElementById("resultsContainer");
const errorState = document.getElementById("errorState");
const errorMessage = document.getElementById("errorMessage");
const gradientPreview = document.getElementById("gradientPreview");
const cssCode = document.getElementById("cssCode");

let currentPrompt = "";

generateBtn.addEventListener("click", generateGradient);
regenerateBtn.addEventListener("click", reimagineGradient);
copyBtn.addEventListener("click", copyToClipboard);

async function generateGradient() {
    const prompt = promptInput.value.trim();

    if (!prompt) {
        showError("Por favor, describe el degradado que deseas generar.");
        return;
    }

    if (!GEMINI_API_KEY || GEMINI_API_KEY === "TU_API_KEY_AQUI") {
        showError("Configura tu API Key de Gemini en script.js.");
        return;
    }

    currentPrompt = prompt;

    hideAll();
    loadingState.style.display = "block";
    generateBtn.disabled = true;

    try {
        const gradient = await callGeminiAPI(prompt);
        displayGradient(gradient);
    } catch (error) {
        console.error("[v0] Error:", error);
        showError(error.message || "Error al generar el degradado.");
    } finally {
        generateBtn.disabled = false;
    }
}

async function callGeminiAPI(prompt) {
    const apiUrl = `https://generativelanguage.googleapis.com/v1/models/${MODEL}:generateContent?key=${GEMINI_API_KEY}`;

    const requestBody = {
        contents: [
            {
                parts: [
                    {
                        text: `Eres un generador de degradados CSS.
Responde ÚNICAMENTE con un valor de degradado CSS.
Ejemplo: linear-gradient(to right, blue, pink)
Prompt del usuario: ${prompt}`
                    }
                ]
            }
        ]
    };

    const response = await fetch(apiUrl, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(requestBody)
    });

    if (!response.ok) {
        const err = await response.json().catch(() => ({}));
        throw new Error(err.error?.message || "Error al comunicarse con Gemini");
    }

    const data = await response.json();
    const responseText = data?.candidates?.[0]?.content?.parts?.[0]?.text ?? "";

    console.log("Respuesta cruda:", responseText);

    const regex = /(linear-gradient\(.*?\)|radial-gradient\(.*?\))/i;
    const match = responseText.match(regex);
    if (!match) throw new Error("No se pudo extraer un degradado válido de la respuesta.");

    return match[1];
}

async function reimagineGradient() {
    if (!currentPrompt) {
        showError("Primero genera un degradado antes de regenerar uno nuevo.");
        return;
    }

    hideAll();
    loadingState.style.display = "block";
    regenerateBtn.disabled = true;

    try {
        const variationPrompt = `${currentPrompt}. Crea una variación creativa diferente, pero mantén el mismo estilo general.`;
        const gradient = await callGeminiAPI(variationPrompt);
        displayGradient(gradient);
    } catch (error) {
        console.error("[v0] Error:", error);
        showError(error.message || "Error al regenerar el degradado.");
    } finally {
        regenerateBtn.disabled = false;
    }
}

function displayGradient(gradient) {
    hideAll();
    resultsContainer.style.display = "block";

    gradientPreview.style.background = gradient;
    cssCode.textContent = `background: ${gradient};`;
}


function copyToClipboard() {
    const text = cssCode.textContent;
    navigator.clipboard.writeText(text).then(() => {
        const original = copyBtn.innerHTML;
        copyBtn.innerHTML = "✅ Copiado";
        setTimeout(() => (copyBtn.innerHTML = original), 2000);
    }).catch(err => showError("No se pudo copiar al portapapeles."));
}


function showError(message) {
    hideAll();
    errorState.style.display = "block";
    errorMessage.textContent = message;
}
function hideAll() {
    loadingState.style.display = "none";
    resultsContainer.style.display = "none";
    errorState.style.display = "none";
}