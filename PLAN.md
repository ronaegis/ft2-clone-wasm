# FastTracker 2 Clone WebAssembly Conversion Plan

## Project Overview

Convert the FastTracker 2 clone (ft2-clone) from a native C/SDL2 application to a WebAssembly module that can be embedded in websites. The goal is to create a JavaScript library that provides core tracker functionality while maintaining pixel-perfect visual fidelity to the original FT2 interface.

## Requirements

- **Deployment Target**: GitHub Pages
- **Integration**: Embeddable in any website
- **Priority**: Core playback functionality first
- **Performance**: No specific requirements (functionality over performance)
- **Visual Fidelity**: Extremely important - must maintain exact FT2 appearance

## Current Architecture Analysis

The ft2-clone is a sophisticated music tracker with several key systems:

### Core Systems
- **Audio Engine**: Real-time XM/MOD/S3M playback with advanced resampling
- **Graphics System**: SDL2-based UI with custom widgets and real-time visualization
- **File I/O**: Support for multiple module and sample formats
- **MIDI Support**: Input/output capabilities
- **Platform Layer**: Windows/Unix-specific APIs for system integration

### Dependencies
- SDL2 (audio, video, input)
- FLAC library (sample loading)
- RtMidi (MIDI support)
- Platform-specific APIs (Windows/Unix)

## Key Challenges for WASM Conversion

1. **Platform Dependencies**: Windows APIs, Unix system calls, and SDL2 need web-compatible replacements
2. **Audio System**: SDL2 audio callback system → Web Audio API
3. **Graphics System**: SDL2 renderer → Canvas 2D or WebGL
4. **File System**: Native file I/O → Browser File APIs
5. **Threading**: SDL threading model → Web Workers (if needed)
6. **Memory Management**: Large audio buffers need optimization for WASM

## Implementation Strategy

### Phase 1: Foundation (Weeks 1-2)

#### 1.1 Set up Emscripten Build System
- Install and configure Emscripten toolchain
- Create CMake toolchain file for Emscripten
- Set up build scripts and compilation flags
- Test basic compilation without SDL dependencies

#### 1.2 Platform Abstraction Layer
- Create `ft2_web_platform.h/c` abstraction layer
- Replace Windows/Unix APIs with web-compatible stubs
- Implement timing functions using browser APIs
- Create configuration storage using localStorage

#### 1.3 Core Engine Isolation
- Identify and isolate core audio/replayer logic
- Remove platform-specific dependencies from core systems
- Create clean interfaces for platform-specific functionality

### Phase 2: Audio System (Weeks 3-4)

#### 2.1 Web Audio API Integration
- Replace SDL2 audio device with `AudioContext`
- Implement audio callback using `ScriptProcessorNode` or `AudioWorkletNode`
- Maintain the sophisticated mixer with all interpolation options
- Implement audio synchronization for real-time playback

#### 2.2 Audio Buffer Management
- Adapt audio buffer allocation for WASM memory constraints
- Implement efficient sample data handling
- Optimize memory usage for large sample sets

#### 2.3 Format Support
- Ensure all module formats (XM, MOD, S3M) work correctly
- Maintain sample format support (WAV, FLAC, AIFF, BRR)
- Test audio quality and synchronization

### Phase 3: Graphics System (Weeks 5-6)

#### 3.1 Canvas 2D Implementation
- Replace SDL2 renderer with Canvas 2D context
- Port all custom GUI widgets (buttons, scrollbars, textboxes, etc.)
- Maintain pixel-perfect FT2 visual style
- Implement efficient rendering for real-time scope visualization

#### 3.2 Widget System
- Port button rendering and interaction
- Implement scrollbar functionality
- Create text input handling
- Maintain exact FT2 color palette and styling

#### 3.3 Real-time Visualization
- Port scope rendering (oscilloscope, spectrum analyzer)
- Implement pattern editor visualization
- Maintain sample editor waveform display
- Ensure smooth real-time updates

### Phase 4: File I/O System (Weeks 7-8)

#### 4.1 Browser File APIs
- Replace native file operations with browser File APIs
- Implement drag-and-drop for modules and samples
- Create virtual file system for module data
- Maintain all format support (XM, MOD, S3M, WAV, FLAC, etc.)

#### 4.2 Module Loading
- Adapt module loader for browser file handling
- Implement sample loading from browser files
- Maintain compatibility with all supported formats
- Create user-friendly file selection interface

### Phase 5: Input System (Weeks 9-10)

#### 5.1 Keyboard Input
- Replace SDL2 keyboard handling with DOM events
- Maintain FT2 keyboard shortcuts and behavior
- Implement text input for pattern editing
- Handle modifier keys and special combinations

#### 5.2 Mouse Input
- Replace SDL2 mouse handling with canvas events
- Implement custom cursor handling
- Maintain drag-and-drop functionality
- Handle mouse wheel and button interactions

#### 5.3 MIDI Support (Optional)
- Implement Web MIDI API support
- Replace RtMidi with browser MIDI API
- Maintain MIDI input/output functionality
- Test with external MIDI devices

### Phase 6: Integration & Polish (Weeks 11-12)

#### 6.1 JavaScript Wrapper
- Create JavaScript API for easy website integration
- Implement module loading and playback controls
- Add event system for UI state changes
- Create documentation and examples

#### 6.2 HTML/CSS Interface
- Design embeddable HTML interface
- Create CSS for pixel-perfect FT2 styling
- Implement responsive design considerations
- Add loading states and error handling

#### 6.3 Memory Optimization
- Optimize WASM memory usage
- Implement efficient data structures
- Add memory cleanup and garbage collection
- Profile and optimize performance bottlenecks

## Technical Specifications

### Build System
- **Toolchain**: Emscripten 3.1+
- **Build System**: CMake with Emscripten toolchain
- **Output**: WebAssembly module + JavaScript wrapper
- **Optimization**: Size over speed (no performance requirements)

### Browser Compatibility
- **Target Browsers**: Chrome 80+, Firefox 75+, Safari 13+, Edge 80+
- **Web Audio API**: Full support required
- **WebAssembly**: Baseline WASM support
- **File API**: Modern File API support

### API Design
```javascript
// Core API for website integration
const tracker = new FT2Clone();
await tracker.initialize();
await tracker.loadModule(file);
tracker.play();
tracker.pause();
// Event system for UI updates
tracker.on('position', (pos) => updateUI(pos));
```

### File Structure
```
ft2-wasm/
├── src/                    # Modified C source
│   ├── ft2_web_platform.c  # Platform abstraction
│   ├── ft2_audio_web.c     # Web Audio implementation
│   ├── ft2_video_web.c     # Canvas graphics
│   └── ...
├── web/                    # Web integration
│   ├── ft2-clone.js        # JavaScript wrapper
│   ├── ft2-clone.css       # Styling
│   ├── index.html          # Demo page
│   └── embed.html          # Embeddable version
├── build/                  # Build scripts
│   ├── build-wasm.sh       # WASM build script
│   └── CMakeLists.txt      # Emscripten CMake
└── docs/                   # Documentation
    ├── API.md              # JavaScript API docs
    └── examples/           # Usage examples
```

## Testing Strategy

### Unit Testing
- Test core audio engine functionality
- Verify module loading and parsing
- Test graphics rendering accuracy
- Validate input handling

### Integration Testing
- Test embedded website integration
- Verify GitHub Pages deployment
- Test with various module files
- Cross-browser compatibility testing

### Visual Testing
- Pixel-perfect comparison with original FT2
- Screenshot-based regression testing
- UI interaction testing
- Real-time visualization testing

## Deployment Strategy

### GitHub Pages
- Static site hosting for demo and documentation
- CDN for WASM files and assets
- Versioned releases with semantic versioning
- Integration examples and documentation

### Distribution
- NPM package for easy installation
- GitHub releases with pre-built binaries
- Documentation and examples
- Community contribution guidelines

## Risk Assessment

### High Risk
- **Audio Quality**: Maintaining audio quality through Web Audio API
- **Visual Fidelity**: Achieving pixel-perfect FT2 appearance
- **Memory Usage**: WASM memory constraints with large samples

### Medium Risk
- **Browser Compatibility**: API differences across browsers
- **File I/O**: Browser security restrictions on file access
- **Performance**: WASM overhead for real-time audio

### Low Risk
- **Core Logic**: Audio engine and replayer are well-isolated
- **Build System**: Emscripten provides good C/WASM tooling
- **JavaScript Integration**: Standard web APIs are well-supported

## Success Metrics

- **Functionality**: Core playback of XM/MOD/S3M files
- **Visual Fidelity**: Pixel-perfect match to FT2 interface
- **Integration**: Easy embedding in websites
- **Compatibility**: Works across target browsers
- **Documentation**: Clear API and usage examples

## Next Steps

1. Begin with Phase 1: Foundation setup
2. Create initial Emscripten build
3. Implement platform abstraction layer
4. Move to audio system implementation
5. Iterate through each phase with testing
6. Final integration and deployment

This plan provides a comprehensive roadmap for converting the FastTracker 2 clone to WebAssembly while maintaining its core functionality and visual fidelity.