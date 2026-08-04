# Qivot Qt Quick Template

A Qivot application with Qt Quick/QML UI using modern CMake.

## Project Structure

```
MyQivotQmlApp/
├── CMakeLists.txt      # Build configuration
├── src/
│   ├── main.cpp        # Application entry point
│   ├── models.h        # Qivot models with Q_GADGET
│   ├── store.h         # QML_ELEMENT store controller
│   └── store.cpp       # Store implementation
├── qml/
│   ├── main.qml        # Qt Quick UI
│   └── qml.qrc         # QML resources
└── README.md           # This file
```

## Prerequisites

- Qt 5.15 or Qt 6 (with QML)
- CMake 3.16+
- C++17 compiler
- Qivot installed

## Building

```bash
# Configure
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/qt/6.x.x

# Build
cmake --build build

# Run
./build/myapp
```

## Architecture

- **models.h**: Qivot models using `Q_GADGET` + `QI_QML_FIELD` for QML binding
- **store.h/store.cpp**: `QML_ELEMENT` controller exposing data to QML
- **main.qml**: Qt Quick declarative UI using ListView bound to QiListModel

The `Store` class uses `QiListModel::setLive()` for reactive updates — any model change automatically refreshes the UI.

## Extending

Add more models to `models.h`:

```cpp
class Note : public QiModel {
    Q_GADGET
    QI_MODEL
public:
    QI_QML_FIELD(QString, title)
    QI_QML_FIELD(QString, content)
};
QI_DECLARE_MODEL(Note, "note", ...);
```

Add more properties and methods to `Store` in `store.h`, then expose them to QML:

```cpp
Q_PROPERTY(QAbstractItemModel *notes READ notes NOTIFY notesChanged)
Q_INVOKABLE void addNote(const QString &title);
```

Update `main.qml` to use the new model.

## Next Steps

- Read [Qivot documentation](https://austinkottke.github.io/Qivot/)
- Check [examples/qmlmodel](https://github.com/austinkottke/Qivot/tree/main/examples/qmlmodel) for more patterns
- See [examples/reactive](https://github.com/austinkottke/Qivot/tree/main/examples/reactive) for live model examples
