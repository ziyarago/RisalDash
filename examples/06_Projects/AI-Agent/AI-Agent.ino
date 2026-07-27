// RisalDash AI-Agent — a talking robot face driven by an LLM.
//
// You type a message in the browser; the ESP32 forwards it to a small PROXY you host, the proxy calls
// an LLM (Claude, GPT, …) and returns a reply plus a mood. The RisalDash "face" widget becomes the
// agent's visible state machine:
//
//     IDLE (neutral) --ask--> LISTENING --> PONDERING --> SPEAKING --> the model's own mood --> IDLE
//                                                    \-> ERROR (on a failed request)
//
// WHY A PROXY?  Never put an LLM API key in firmware — anyone with the .bin can read it, and the ESP
// can't safely pin the vendor's TLS chain anyway. The proxy holds the secret key, does the vendor
// call, and hands the ESP a tiny JSON. It's ~15 lines (a Cloudflare Worker / a Node route) — see the
// contract at the bottom of this file.
//
// Board: ESP32 family (needs WiFiClientSecure + a bit of RAM). Open the dashboard, go to "Agent",
// type, and watch the face react. The same mood also drives an on-device LCD if you add eyes().

#ifndef USE_PORTAL
#define USE_PORTAL 1        // 1 = join YOUR Wi-Fi (needed — the agent calls the internet)
#endif
#include <RisalUI.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ── Your proxy endpoint (NOT the LLM vendor directly). It holds the API key. ──
static const char* PROXY_URL = "https://your-worker.example.workers.dev/agent";

RisalUI dash("AI Agent");

// ── Bound values (the dashboard reads/writes these over the WebSocket) ──
String userMsg = "";                 // what you type
String reply   = "Ask me anything.";  // the agent's answer (shown in the AI card)
int    mood    = FACE_NEUTRAL;        // the face — our state machine's visible output

// ── Agent state machine ──
bool     askPending = false;          // set by the "Ask" button, handled in loop()
uint32_t faceUntil  = 0;              // when the current transient mood should fall back
int      restMood   = FACE_NEUTRAL;   // mood to settle into after SPEAKING

// Pull a "reply" string and an integer "mood" out of the proxy's JSON without a JSON library.
// Expects: {"reply":"...","mood":12}. Good enough for a demo; use ArduinoJson for anything real.
static void parseAgentJson(const String& body, String& outReply, int& outMood) {
  int rk = body.indexOf("\"reply\"");
  if (rk >= 0) {
    int q1 = body.indexOf('"', body.indexOf(':', rk) + 1);
    int q2 = q1 >= 0 ? body.indexOf('"', q1 + 1) : -1;
    if (q2 > q1) outReply = body.substring(q1 + 1, q2);
  }
  int mk = body.indexOf("\"mood\"");
  if (mk >= 0) outMood = body.substring(body.indexOf(':', mk) + 1).toInt();
}

// Blocking POST to the proxy. Returns the HTTP status (200 = ok). Keep messages short on an ESP.
static int askAgent(const String& msg, String& outReply, int& outMood) {
  WiFiClientSecure net;
  net.setInsecure();               // demo: skip cert validation. Pin your proxy's CA for production.
  HTTPClient http;
  if (!http.begin(net, PROXY_URL)) return -1;
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"message\":\"" + msg + "\"}";   // (escape quotes for real input)
  int code = http.POST(payload);
  if (code == 200) parseAgentJson(http.getString(), outReply, outMood);
  http.end();
  return code;
}

void setup() {
  dash.brand("Risal<b>Dash</b>");

  dash.layout("Agent", RICON_MOTION);
  dash.face("Robot", &mood).size(RSIZE_L);        // the big animated face — the agent's expression
  dash.text("Message", &userMsg);                 // type here
  dash.button("Ask", "Ask the agent", []() {      // press -> handled in loop() (no network in a callback)
    if (userMsg.length()) askPending = true;
  });
  dash.ai("Reply", &reply);                        // the answer, rendered as an assistant card

#if USE_PORTAL
  dash.begin();                                    // first boot -> captive portal -> joins your Wi-Fi
#else
  dash.beginAP("RisalDash-Agent", "12345678");     // offline AP (the agent call will fail without internet)
#endif
}

void loop() {
  dash.loop();
  uint32_t now = millis();

  // 1) A question was asked — run the state machine.
  if (askPending) {
    askPending = false;

    mood = FACE_LISTENING; reply = "…"; dash.loop();   // flush LISTENING before we block
    mood = FACE_PONDERING;               dash.loop();   // ...then "thinking"

    String r; int m = FACE_HAPPY;
    int code = askAgent(userMsg, r, m);

    if (code == 200) {
      reply    = r.length() ? r : "(empty reply)";
      restMood = (m >= 0 && m < FACE_COUNT) ? m : FACE_HAPPY;  // the model's chosen emotion
      mood     = FACE_SPEAKING;                                 // "talking" first...
      faceUntil = now + 2200;                                   // ...then settle into restMood
    } else {
      reply    = "Request failed (" + String(code) + "). Is the proxy reachable?";
      mood     = FACE_ERROR;
      restMood = FACE_NEUTRAL;
      faceUntil = now + 2600;
    }
  }

  // 2) Let transient moods (SPEAKING / ERROR) fall back on a timer.
  if (faceUntil && now > faceUntil) {
    if (mood == FACE_SPEAKING) { mood = restMood; faceUntil = now + 3500; }  // hold the emotion a bit
    else                       { mood = FACE_NEUTRAL; faceUntil = 0; }        // back to resting face
  }
}

// ────────────────────────────────────────────────────────────────────────────────────────────────
// PROXY CONTRACT — host this yourself; it keeps the API key off the device.
//
//   POST /agent   { "message": "turn on the porch light" }
//   200 OK        { "reply": "Done — porch light is on.", "mood": 35 }   // 35 = FACE_SUCCESS
//
// Minimal Cloudflare Worker (pseudo):
//   export default { async fetch(req, env) {
//     const { message } = await req.json();
//     const r = await fetch("https://api.anthropic.com/v1/messages", {
//       method: "POST",
//       headers: { "x-api-key": env.ANTHROPIC_API_KEY, "anthropic-version": "2023-06-01",
//                  "content-type": "application/json" },
//       body: JSON.stringify({ model: "claude-sonnet-5", max_tokens: 200,
//         system: "Reply briefly. End with a line MOOD=<0..41> for the robot face.",
//         messages: [{ role: "user", content: message }] }) });
//     const data = await r.json();
//     const text = data.content?.[0]?.text ?? "";
//     const mood = parseInt((text.match(/MOOD=(\d+)/) || [])[1] ?? "1");
//     return Response.json({ reply: text.replace(/\s*MOOD=\d+\s*$/, ""), mood });
//   }};
//
// Mood indices come from the FaceMood enum (0 NEUTRAL … 41 BATTERY). Handy ones for an agent:
//   10 LISTENING · 11 PONDERING · 32 SPEAKING · 35 SUCCESS · 36 ERROR · 1 HAPPY · 19 WORRIED.
