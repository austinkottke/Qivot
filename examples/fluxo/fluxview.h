#ifndef FLUXVIEW_H
#define FLUXVIEW_H

#include <QQuickPaintedItem>
#include <QQmlEngine>          // QML_ELEMENT
#include <QImage>
#include <QVector>
#include <QElapsedTimer>
#include <QMutex>
#include <qivot.h>

class QTimer;

/// The whole demo in one item.
///
/// A flow-field particle system drifts across a dark canvas leaving glowing
/// trails. Every frame, the position of **every** particle is batch-written to
/// SQLite in a single transaction (WAL) — a sustained firehose of writes. A
/// timeline scrubber then reads any past frame straight back out of the
/// database and replays it, so the picture on screen is literally reconstructed
/// from rows. The prettier it moves, the harder it writes.
class FluxView : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool      running       READ running       WRITE setRunning       NOTIFY runningChanged)
    Q_PROPERTY(int       particleCount READ particleCount WRITE setParticleCount NOTIFY particleCountChanged)
    Q_PROPERTY(bool      replaying     READ replaying                            NOTIFY replayingChanged)
    Q_PROPERTY(qlonglong rowsWritten   READ rowsWritten                          NOTIFY statsChanged)
    Q_PROPERTY(int       writesPerSec  READ writesPerSec                         NOTIFY statsChanged)
    Q_PROPERTY(qlonglong liveRows      READ liveRows                             NOTIFY statsChanged)
    Q_PROPERTY(double    dbSizeMB      READ dbSizeMB                             NOTIFY statsChanged)
    Q_PROPERTY(double    batchMs       READ batchMs                              NOTIFY statsChanged)
    Q_PROPERTY(int       headFrame     READ headFrame                            NOTIFY frameRangeChanged)
    Q_PROPERTY(int       minFrame      READ minFrame                             NOTIFY frameRangeChanged)
    Q_PROPERTY(int       scrubFrame    READ scrubFrame    WRITE setScrubFrame    NOTIFY scrubFrameChanged)

public:
    explicit FluxView(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    bool      running() const       { return m_running; }
    void      setRunning(bool r);
    int       particleCount() const { return m_count; }
    void      setParticleCount(int n);
    bool      replaying() const     { return m_replaying; }
    qlonglong rowsWritten() const   { return m_rowsWritten; }
    int       writesPerSec() const  { return m_writesPerSec; }
    qlonglong liveRows() const      { return m_liveRows; }
    double    dbSizeMB() const      { return m_dbSizeMB; }
    double    batchMs() const       { return m_batchMs; }
    int       headFrame() const     { return m_headFrame; }
    int       minFrame() const      { return m_minFrame; }
    int       scrubFrame() const    { return m_scrubFrame; }
    void      setScrubFrame(int f);

    /// Leave replay and resume recording live at the head of history.
    Q_INVOKABLE void live();
    /// Wipe all history and start over.
    Q_INVOKABLE void clear();

    /// Headless: run `frames` steps of the sim into an internal buffer and save
    /// the result to `path` — used to generate the screenshot without a window.
    void headlessRender(int w, int h, int frames, const QString &path);

signals:
    void runningChanged();
    void particleCountChanged();
    void replayingChanged();
    void statsChanged();
    void frameRangeChanged();
    void scrubFrameChanged();

private slots:
    void tick();

private:
    void  ensureImage();                 // (re)allocate the trail buffer on resize
    void  seedParticles();               // fill the pool with fresh, randomized particles
    void  respawn(int i);                // recycle one particle to a random spot
    void  stepAndDraw();                 // advance the sim + composite one frame
    void  writeFrame();                  // batch-write this frame's particles to SQLite
    void  renderReplay(int frame);       // rebuild the canvas from DB history
    void  refreshDbStats();              // page_count/page_size + writes/sec
    float flow(float x, float y, float t) const;   // flow-field angle at a point
    float noise(float x, float y, float z) const;  // 3D Perlin noise

    // --- simulation state ---
    QVector<float> m_x, m_y;             // particle positions
    QVector<float> m_life;               // frames of life left
    QVector<int>   m_hue;                // per-particle color
    int   m_count = 1800;
    float m_time  = 0.f;
    quint32 m_rng = 0x9e3779b9u;         // tiny deterministic PRNG state

    // --- presentation ---
    QImage  m_trail;                     // persistent glow buffer (fades each frame)
    QMutex  m_imgMutex;                  // guards m_trail across GUI/render threads
    QTimer *m_timer = nullptr;

    // --- recording / stats ---
    QiConnection m_conn;
    bool      m_running     = true;
    bool      m_replaying   = false;
    int       m_headFrame   = 0;         // next frame index to write
    int       m_minFrame    = 0;         // oldest frame still in the table
    int       m_scrubFrame  = 0;
    qlonglong m_rowsWritten = 0;         // cumulative — never resets while running
    qlonglong m_liveRows    = 0;         // rows currently in the table
    int       m_writesPerSec = 0;
    double    m_dbSizeMB    = 0.0;
    double    m_batchMs     = 0.0;

    // writes/sec sampling
    QElapsedTimer m_rateClock;
    qlonglong     m_rowsAtSample = 0;

    // Perlin permutation table (duplicated to 512 to avoid index wrapping)
    int m_perm[512];
};

#endif // FLUXVIEW_H
