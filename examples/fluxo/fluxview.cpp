#include "fluxview.h"
#include "sample.h"

#include <QPainter>
#include <QTimer>
#include <QtMath>
#include <algorithm>   // std::swap

// ---- tunables -------------------------------------------------------------
static constexpr int   kRetentionFrames = 1400;  // keep ~23s of history at 60fps
static constexpr int   kReplayTrail     = 18;    // frames composited when scrubbing
static constexpr float kSpeed           = 1.5f;  // particle step length (px/frame)
static constexpr float kFieldScale      = 0.0016f;
static constexpr int   kFadeAlpha       = 30;    // trail persistence (lower = longer)
static const QColor    kBg              = QColor(6, 7, 15);

// ---- tiny deterministic PRNG (no Math.random / global state) --------------
static inline float frand(quint32 &s) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (s & 0xffffffu) / float(0x1000000);
}

// ---- Perlin noise helpers -------------------------------------------------
static inline float pfade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
static inline float plerp(float a, float b, float t) { return a + t * (b - a); }
static inline float pgrad(int hash, float x, float y, float z) {
    const int h = hash & 15;
    const float u = h < 8 ? x : y;
    const float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

FluxView::FluxView(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setFillColor(kBg);
    setOpaquePainting(true);

    // Seed the Perlin permutation table deterministically.
    quint32 s = 1337u;
    int p[256];
    for (int i = 0; i < 256; i++) p[i] = i;
    for (int i = 255; i > 0; i--) {
        int j = int(frand(s) * (i + 1));
        std::swap(p[i], p[j]);
    }
    for (int i = 0; i < 512; i++) m_perm[i] = p[i & 255];

    m_conn = QiConnection::defaultConnection();
    m_rateClock.start();

    m_timer = new QTimer(this);
    m_timer->setInterval(16);   // ~60 fps
    connect(m_timer, &QTimer::timeout, this, &FluxView::tick);
    m_timer->start();
}

float FluxView::noise(float x, float y, float z) const {
    const int X = int(std::floor(x)) & 255;
    const int Y = int(std::floor(y)) & 255;
    const int Z = int(std::floor(z)) & 255;
    x -= std::floor(x); y -= std::floor(y); z -= std::floor(z);
    const float u = pfade(x), v = pfade(y), w = pfade(z);
    const int A = m_perm[X] + Y, AA = m_perm[A] + Z, AB = m_perm[A + 1] + Z;
    const int B = m_perm[X + 1] + Y, BA = m_perm[B] + Z, BB = m_perm[B + 1] + Z;

    const float x1 = plerp(pgrad(m_perm[AA],     x,     y, z), pgrad(m_perm[BA],     x - 1,     y, z), u);
    const float x2 = plerp(pgrad(m_perm[AB],     x, y - 1, z), pgrad(m_perm[BB],     x - 1, y - 1, z), u);
    const float y1 = plerp(x1, x2, v);
    const float x3 = plerp(pgrad(m_perm[AA + 1], x,     y, z - 1), pgrad(m_perm[BA + 1], x - 1,     y, z - 1), u);
    const float x4 = plerp(pgrad(m_perm[AB + 1], x, y - 1, z - 1), pgrad(m_perm[BB + 1], x - 1, y - 1, z - 1), u);
    const float y2 = plerp(x3, x4, v);
    return plerp(y1, y2, w);   // ~[-1, 1]
}

float FluxView::flow(float x, float y, float t) const {
    // Map smooth noise to an angle; the *2.5 gives the field some swirl.
    return noise(x * kFieldScale, y * kFieldScale, t * 0.6f) * float(M_PI) * 2.5f;
}

void FluxView::respawn(int i) {
    const float W = float(width()), H = float(height());
    m_x[i] = frand(m_rng) * W;
    m_y[i] = frand(m_rng) * H;
    m_life[i] = 60.f + frand(m_rng) * 220.f;
    // Neon spread: cyan → blue → violet → magenta across the field, drifting
    // slowly over time so the whole palette breathes.
    const int base = int(m_time * 12.f);
    const float fx = m_x[i] / qMax(1.f, W);
    const float fy = m_y[i] / qMax(1.f, float(height()));
    m_hue[i] = (185 + base + int(fx * 150.f) + int(fy * 40.f) + int(frand(m_rng) * 24.f)) % 360;
}

void FluxView::seedParticles() {
    m_x.resize(m_count); m_y.resize(m_count);
    m_life.resize(m_count); m_hue.resize(m_count);
    for (int i = 0; i < m_count; i++) respawn(i);
}

void FluxView::ensureImage() {
    const int W = int(width()), H = int(height());
    if (W <= 0 || H <= 0) return;
    if (m_trail.width() != W || m_trail.height() != H) {
        QMutexLocker lk(&m_imgMutex);
        QImage img(W, H, QImage::Format_ARGB32_Premultiplied);
        img.fill(kBg);
        m_trail = img;
    }
    if (m_x.size() != m_count) seedParticles();
}

void FluxView::stepAndDraw() {
    const float W = float(width()), H = float(height());

    QMutexLocker lk(&m_imgMutex);
    QPainter g(&m_trail);

    // 1) Fade the whole buffer a touch toward the background — this is what
    //    turns moving dots into glowing trails.
    g.setCompositionMode(QPainter::CompositionMode_SourceOver);
    g.fillRect(m_trail.rect(), QColor(kBg.red(), kBg.green(), kBg.blue(), kFadeAlpha));

    // 2) Advance every particle through the flow field and draw its glow.
    g.setCompositionMode(QPainter::CompositionMode_Plus);
    g.setPen(Qt::NoPen);
    g.setRenderHint(QPainter::Antialiasing, true);
    for (int i = 0; i < m_count; i++) {
        const float a = flow(m_x[i], m_y[i], m_time);
        m_x[i] += std::cos(a) * kSpeed;
        m_y[i] += std::sin(a) * kSpeed;
        m_life[i] -= 1.f;
        if (m_life[i] <= 0.f || m_x[i] < 0 || m_x[i] >= W || m_y[i] < 0 || m_y[i] >= H) {
            respawn(i);
        }
        QColor c = QColor::fromHsvF(m_hue[i] / 360.0, 0.72, 1.0, 0.55);
        g.setBrush(c);
        g.drawEllipse(QPointF(m_x[i], m_y[i]), 1.5, 1.5);
    }
    g.end();
    m_time += 0.006f;
}

void FluxView::writeFrame() {
    // Build one QiList and batch-save it — a single prepared, transactional
    // multi-row INSERT. This is the write firehose.
    QiList<Sample> batch;
    QiListWriter w(&batch);
    for (int i = 0; i < m_count; i++)
        w << m_headFrame << double(m_x[i]) << double(m_y[i]) << m_hue[i] << w.next();

    QElapsedTimer t; t.start();
    batch.save();
    m_batchMs = t.nsecsElapsed() / 1.0e6;

    m_rowsWritten += m_count;
    m_headFrame   += 1;

    // Retention: drop the frame that just fell out of the window, so the table
    // stays bounded while the cumulative write counter keeps climbing.
    const int cutoff = m_headFrame - kRetentionFrames;
    if (cutoff > m_minFrame) {
        (void) QiQuery<Sample>().filter(QiWhere("frame < ", cutoff)).remove();
        m_minFrame = cutoff;
    }
    m_liveRows = qint64(m_headFrame - m_minFrame) * m_count;
}

void FluxView::renderReplay(int frame) {
    const int lo = qMax(m_minFrame, frame - kReplayTrail + 1);

    QMutexLocker lk(&m_imgMutex);
    if (m_trail.isNull()) return;
    m_trail.fill(kBg);
    QPainter g(&m_trail);
    g.setCompositionMode(QPainter::CompositionMode_Plus);
    g.setPen(Qt::NoPen);
    g.setRenderHint(QPainter::Antialiasing, true);

    // Composite a window of frames read straight back from SQLite — oldest
    // faintest, newest brightest — reconstructing the trailed look from rows.
    for (int f = lo; f <= frame; f++) {
        const float k = 1.f - float(frame - f) / float(kReplayTrail);
        QiList<Sample> rows = QiQuery<Sample>().filter(QiWhere("frame = ", f)).all();
        for (int i = 0; i < rows.size(); i++) {
            Sample *s = rows.at(i);
            if (!s) continue;
            QColor c = QColor::fromHsvF(s->hue.get().toInt() / 360.0, 0.72, 1.0, 0.55 * k);
            g.setBrush(c);
            g.drawEllipse(QPointF(s->x.get().toDouble(), s->y.get().toDouble()), 1.5, 1.5);
        }
    }
    g.end();
}

void FluxView::refreshDbStats() {
    // writes/sec, sampled over a moving ~half-second window.
    const qint64 ms = m_rateClock.elapsed();
    if (ms >= 500) {
        m_writesPerSec = int((m_rowsWritten - m_rowsAtSample) * 1000 / ms);
        m_rowsAtSample = m_rowsWritten;
        m_rateClock.restart();
    }
    // Logical DB size = page_count * page_size (counts pages held in the WAL too).
    QSqlQuery q = m_conn.query();
    if (q.exec("PRAGMA page_count") && q.next()) {
        const qint64 pages = q.value(0).toLongLong();
        if (q.exec("PRAGMA page_size") && q.next())
            m_dbSizeMB = pages * q.value(0).toDouble() / (1024.0 * 1024.0);
    }
}

void FluxView::tick() {
    ensureImage();
    if (m_trail.isNull()) return;
    if (m_replaying || !m_running) return;

    stepAndDraw();
    writeFrame();

    static int n = 0;
    if (++n % 12 == 0) { refreshDbStats(); emit statsChanged(); emit frameRangeChanged(); }

    m_scrubFrame = m_headFrame - 1;
    emit scrubFrameChanged();
    update();
}

void FluxView::paint(QPainter *painter) {
    QMutexLocker lk(&m_imgMutex);
    if (!m_trail.isNull()) painter->drawImage(0, 0, m_trail);
}

// ---- properties / controls ------------------------------------------------
void FluxView::setRunning(bool r) {
    if (m_running == r) return;
    m_running = r;
    if (r && m_replaying) { m_replaying = false; emit replayingChanged(); }
    emit runningChanged();
}

void FluxView::setParticleCount(int n) {
    n = qBound(100, n, 6000);
    if (n == m_count) return;
    m_count = n;
    seedParticles();
    emit particleCountChanged();
}

void FluxView::setScrubFrame(int f) {
    if (m_headFrame == 0) return;
    f = qBound(m_minFrame, f, m_headFrame - 1);
    m_scrubFrame = f;
    if (!m_replaying) { m_replaying = true; emit replayingChanged(); }
    renderReplay(f);
    update();
    emit scrubFrameChanged();
}

void FluxView::live() {
    if (!m_replaying) return;
    m_replaying = false;
    m_scrubFrame = qMax(0, m_headFrame - 1);
    emit replayingChanged();
    emit scrubFrameChanged();
}

void FluxView::headlessRender(int w, int h, int frames, const QString &path) {
    setWidth(w); setHeight(h);
    ensureImage();
    for (int i = 0; i < frames; i++) stepAndDraw();
    QMutexLocker lk(&m_imgMutex);
    m_trail.save(path);
}

void FluxView::clear() {
    (void) m_conn.query().exec("DELETE FROM sample");
    m_headFrame = 0; m_minFrame = 0; m_rowsWritten = 0; m_liveRows = 0;
    m_rowsAtSample = 0; m_scrubFrame = 0; m_time = 0.f;
    m_replaying = false;
    seedParticles();
    { QMutexLocker lk(&m_imgMutex); if (!m_trail.isNull()) m_trail.fill(kBg); }
    emit replayingChanged(); emit statsChanged();
    emit frameRangeChanged(); emit scrubFrameChanged();
    update();
}
