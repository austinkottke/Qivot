#!/bin/bash
#
# Qivot Project Generator
# Creates a new Qivot project from templates
#
# Usage: ./create-qivot-project.sh MyProject [--cmake|--qmake] [--qml]
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMPLATES_DIR="$(dirname "$SCRIPT_DIR")/templates"

# Show help
show_help() {
    cat <<EOF
Qivot Project Generator

Usage: $0 PROJECT_NAME [OPTIONS]

OPTIONS:
  --cmake          Use CMake build system (default)
  --qmake          Use qmake build system
  --qml            Include Qt Quick/QML support
  --help           Show this help message

EXAMPLES:
  $0 MyApp
  $0 MyApp --cmake --qml
  $0 MyApp --qmake

EOF
}

# Parse arguments
if [[ $# -eq 0 ]]; then
    echo "Error: Project name required"
    show_help
    exit 1
fi

PROJECT_NAME="$1"
BUILD_SYSTEM="cmake"  # default
WITH_QML=false

# Parse options
shift
while [[ $# -gt 0 ]]; do
    case "$1" in
        --cmake)
            BUILD_SYSTEM="cmake"
            shift
            ;;
        --qmake)
            BUILD_SYSTEM="qmake"
            shift
            ;;
        --qml)
            WITH_QML=true
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            echo "Error: Unknown option $1"
            show_help
            exit 1
            ;;
    esac
done

# Validate project name
if ! [[ "$PROJECT_NAME" =~ ^[A-Za-z][A-Za-z0-9_]*$ ]]; then
    echo -e "${RED}Error: Invalid project name. Must start with letter, contain only alphanumeric and underscore.${NC}"
    exit 1
fi

# Check if directory exists
if [[ -d "$PROJECT_NAME" ]]; then
    echo -e "${RED}Error: Directory '$PROJECT_NAME' already exists${NC}"
    exit 1
fi

# Select template path
if [[ "$BUILD_SYSTEM" == "cmake" ]]; then
    if [[ "$WITH_QML" == true ]]; then
        TEMPLATE_SRC="$TEMPLATES_DIR/cmake/MyQivotQmlApp"
    else
        TEMPLATE_SRC="$TEMPLATES_DIR/cmake/MyQivotProject"
    fi
elif [[ "$BUILD_SYSTEM" == "qmake" ]]; then
    TEMPLATE_SRC="$TEMPLATES_DIR/qmake"
fi

# Check template exists
if [[ ! -d "$TEMPLATE_SRC" ]]; then
    echo -e "${RED}Error: Template not found at $TEMPLATE_SRC${NC}"
    exit 1
fi

# Create project
echo -e "${BLUE}Creating Qivot project: $PROJECT_NAME${NC}"
echo "  Build system: $BUILD_SYSTEM"
echo "  Qt Quick: $([[ "$WITH_QML" == true ]] && echo "Yes" || echo "No")"
echo ""

# Copy template
mkdir -p "$PROJECT_NAME"
cp -r "$TEMPLATE_SRC"/* "$PROJECT_NAME/"

# Special handling for qmake - template is just a .pro file
if [[ "$BUILD_SYSTEM" == "qmake" && ! -d "$TEMPLATE_SRC/src" ]]; then
    mkdir -p "$PROJECT_NAME/src"

    # Copy models.h and main.cpp from cmake template
    cp "$TEMPLATES_DIR/cmake/MyQivotProject/src/models.h" "$PROJECT_NAME/src/"
    cp "$TEMPLATES_DIR/cmake/MyQivotProject/src/main.cpp" "$PROJECT_NAME/src/"
fi

# Rename executable in CMakeLists.txt if needed
if [[ "$BUILD_SYSTEM" == "cmake" ]]; then
    if [[ -f "$PROJECT_NAME/CMakeLists.txt" ]]; then
        # Use perl-compatible regex with different delimiter to avoid conflicts
        if [[ "$OSTYPE" == "darwin"* ]]; then
            sed -i '' "s|project(MyQivot.*)|project($PROJECT_NAME)|" "$PROJECT_NAME/CMakeLists.txt"
            sed -i '' "s|myapp|$PROJECT_NAME|g" "$PROJECT_NAME/CMakeLists.txt"
        else
            sed -i "s|project(MyQivot.*)|project($PROJECT_NAME)|" "$PROJECT_NAME/CMakeLists.txt"
            sed -i "s|myapp|$PROJECT_NAME|g" "$PROJECT_NAME/CMakeLists.txt"
        fi
    fi
fi

# Rename .pro file if using qmake
if [[ "$BUILD_SYSTEM" == "qmake" ]]; then
    if [[ -f "$PROJECT_NAME/MyQivotProject.pro" ]]; then
        mv "$PROJECT_NAME/MyQivotProject.pro" "$PROJECT_NAME/$PROJECT_NAME.pro"
        if [[ "$OSTYPE" == "darwin"* ]]; then
            sed -i '' "s|myapp|$PROJECT_NAME|g" "$PROJECT_NAME/$PROJECT_NAME.pro"
        else
            sed -i "s|myapp|$PROJECT_NAME|g" "$PROJECT_NAME/$PROJECT_NAME.pro"
        fi
    fi
fi

# Create build directory
mkdir -p "$PROJECT_NAME/build"

echo -e "${GREEN}✓ Project created!${NC}"
echo ""
echo "Next steps:"
echo "  cd $PROJECT_NAME"
echo ""

if [[ "$BUILD_SYSTEM" == "cmake" ]]; then
    echo "  # Configure (adjust Qt path as needed)"
    echo "  cd build"
    echo "  cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt/6.x.x"
    echo ""
    echo "  # Build and run"
    echo "  cmake --build ."
    echo "  ./$PROJECT_NAME"
elif [[ "$BUILD_SYSTEM" == "qmake" ]]; then
    echo "  # Build"
    echo "  qmake"
    echo "  make"
    echo ""
    echo "  # Run"
    echo "  ./$PROJECT_NAME"
fi

echo ""
echo "Documentation:"
echo "  - Edit src/models.h to define your models"
echo "  - Use qivot-gen to generate models from existing databases:"
echo "    python3 qivot-gen.py --db sqlite:mydb.db --output src/models.h"
echo "  - Visit https://austinkottke.github.io/Qivot/ for docs"
echo ""
