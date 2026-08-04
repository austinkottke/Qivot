# Qivot Project Template

A minimal Qivot C++ application using modern CMake.

## Project Structure

```
MyQivotProject/
├── CMakeLists.txt    # Build configuration
├── src/
│   ├── main.cpp      # Application entry point
│   └── models.h      # Qivot model definitions (edit this)
└── README.md         # This file
```

## Prerequisites

- Qt 5.15 or Qt 6
- CMake 3.16+
- C++17 compiler
- Qivot installed (via package manager or built from source)

## Building

```bash
# Configure the project
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/qt/6.x.x

# Build
cmake --build build

# Run
./build/myapp
```

## Customizing Models

Edit `src/models.h` to define your Qivot models. For example:

```cpp
class Product : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<qreal>   price;
};
QI_DECLARE_MODEL(Product, "product",
                 QI_FIELD(name, QiNotNull),
                 QI_FIELD(price));
```

Then use them in `src/main.cpp`:

```cpp
connection.addModel<Product>();
connection.createTables();

Product p;
p.name = "Widget";
p.price = 19.99;
p.save();
```

## Next Steps

- Read the [Qivot documentation](https://austinkottke.github.io/Qivot/)
- Explore [example projects](https://github.com/austinkottke/Qivot/tree/main/examples)
- Use `qivot-gen` to scaffold models from existing databases:
  ```bash
  python3 qivot-gen.py --db sqlite:mydb.db --output src/models.h
  ```
