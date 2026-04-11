# X-TRACK Core Logic Analysis

## Project Overview
**X-TRACK** is a multi-platform GPS tracking device firmware written in C++, featuring a sophisticated MVP (Model-View-Presenter) architecture with support for embedded systems (STM32F4), Linux/SDL2 simulator, and Windows simulator.

---

## 1. Architecture Overview

### High-Level Structure
```
┌─────────────────────────────────────────────────────────────┐
│                    MAIN ENTRY POINT                         │
│  (main.cpp) - Core_Init() → setup() → loop()               │
└────────────┬────────────────────────────────────────────────┘
             │
    ┌────────┴────────┐
    │                 │
    ▼                 ▼
┌──────────┐      ┌──────────────────┐
│   HAL    │      │   LVGL GUI       │
│ (H/W)    │      │  (Display/UI)    │
└──────────┘      └──────────────────┘
    │                 ▲
    │    ┌────────────┘
    │    │
    └────┴──────────────┐
                        │
                   ┌────▼────────────────┐
                   │  DataProc + Pages   │
                   │  (Application)      │
                   └─────────────────────┘
```

### Main Entry Flow
```cpp
int main() {
    Core_Init();           // Platform-specific initialization
    setup();              // Application setup
    for(;;) loop();       // Main loop
}

void setup() {
    HAL::HAL_Init();                    // Initialize hardware (GPS, IMU, I2C, etc.)
    lv_init();                          // Initialize LVGL graphics library
    lv_port_init();                     // Initialize LVGL port/display
    App_Init();                         // Initialize application
    HAL::Power_SetEventCallback(App_Uninit);  // Set power-off callback
}

void loop() {
    HAL::HAL_Update();                  // Update hardware (read sensors, GPS, etc.)
    lv_task_handler();                  // Process LVGL GUI tasks
    __wfi();                            // Wait for interrupt (low power)
}
```

---

## 2. Core Components

### 2.1 Hardware Abstraction Layer (HAL)
**Location:** `USER/HAL/`

Provides platform-independent interface to hardware:

| Component | File | Purpose |
|-----------|------|---------|
| **GPS** | `HAL_GPS.cpp` | GNSS/GPS positioning, satellite tracking |
| **IMU** | `HAL_IMU.cpp` | 6-axis inertial measurement (acceleration, gyroscope) |
| **Magnetometer** | `HAL_MAG.cpp` | Compass/magnetic field measurement |
| **Clock** | `HAL_Clock.cpp` | Real-time clock management |
| **Power** | `HAL_Backlight.cpp` | Power management, backlight control |
| **Audio** | `HAL_Audio.cpp` + `HAL_Buzz.cpp` | Sound and buzzer output |
| **Encoder** | `HAL_Encoder.cpp` | Rotary encoder input |
| **I2C/Communication** | `HAL_I2C_Scan.cpp` | I2C bus scanning and communication |
| **AHRS** | `HAL_AHRS.cpp` | Attitude & Heading Reference System (sensor fusion) |
| **SD Card** | `HAL_SD_CARD.cpp` | External SD card storage |
| **Memory** | `HAL_Memory.cpp` | Memory management and diagnostics |

**Key HAL Structures:**
```cpp
HAL::GPS_Info_t         // GPS data (lat, lon, speed, heading, satellites)
HAL::IMU_Info_t         // IMU data (accel, gyro values)
HAL::MAG_Info_t         // Magnetometer data
HAL::Clock_Info_t       // Time information
```

### 2.2 Data Processing & Pub/Sub System
**Location:** `USER/App/Common/DataProc/`

Uses **DataCenter** - a lock-free publish/subscribe pattern:

**Key Data Processors:**
| Processor | File | Data Stream | Function |
|-----------|------|-------------|----------|
| **GPS** | `DP_GPS.cpp` | HAL_GPS_Info → Application | Monitors GPS quality (0-3 satellites), publishes valid GPS data, plays audio cues |
| **IMU** | `DP_IMU.cpp` | HAL_IMU_Info → Application | Real-time acceleration/gyroscope data |
| **MAG** | `DP_MAG.cpp` | HAL_MAG_Info → Application | Magnetometer compass data |
| **Clock** | `DP_Clock.cpp` | System time | Time tracking and formatting |
| **Recorder** | `DP_Recorder.cpp` | Track data → SD Card | Records GPS track points to storage |
| **TrackFilter** | `DP_TrackFilter.cpp` | GPS points → Filtered track | Removes noise from GPS tracks (Douglas-Peucker algorithm) |
| **SportStatus** | `DP_SportStatus.cpp` | Movement data → Stats | Step counter, distance, speed calculations |
| **Power** | `DP_Power.cpp` | Battery/Power events | System power state management |
| **Storage** | `DP_Storage.cpp` | Persistent data ↔ SD Card | Key-value storage system (int, float, string) |
| **SysConfig** | `DP_SysConfig.cpp` | Settings → Storage | System configuration (timezone, language, maps) |
| **MusicPlayer** | `DP_MusicPlayer.cpp` | Audio playback | Plays audio feedback/music files |
| **TzConv** | `DP_TzConv.cpp` | UTC → Local time | Timezone conversion |

**Data Flow Pattern:**
```cpp
// GPS data processor example
void onTimer(Account* account) {
    HAL::GPS_GetInfo(&gpsInfo);
    
    // Quality check (need ≥3 satellites)
    if (satellites >= 3) {
        account->Commit(&gpsInfo, sizeof(gpsInfo));  // Write to shared state
        account->Publish();                           // Notify subscribers
    }
}
```

### 2.3 LVGL GUI Framework
**Location:** `USER/App/Pages/`

Uses LVGL (Light and Versatile Graphics Library) v8.0.0+ with custom resource pool and page manager.

**Page Structure:**
| Page | Location | Purpose |
|------|----------|---------|
| **Startup** | `Pages/StartUp/` | Splash screen on boot |
| **Dialplate** | `Pages/Dialplate/` | Digital watch face display |
| **LiveMap** | `Pages/LiveMap/` | Real-time GPS map with heading indicator |
| **SystemInfos** | `Pages/SystemInfos/` | Device info, stats, diagnostics |
| **StatusBar** | `Pages/StatusBar/` | Top bar (time, battery, GPS status) |
| **Template** | `Pages/_Template/` | Page creation template |

**Page Manager Features:**
- MVP (Model-View-Presenter) pattern implementation
- Page stack-based navigation
- Animation transitions (LOAD_ANIM_OVER_TOP)
- Resource pooling for fonts/images

---

## 3. Data Flow: Example - GPS Tracking

### Complete Flow: Satellite Detection → Display Update

```
1. HAL Layer (Hardware)
   ↓
   HAL::GPS_GetInfo() reads GNSS module
   └─ Returns: GPS_Info_t {latitude, longitude, satellites, speed, heading}

2. Data Processing Layer
   ↓
   DP_GPS::onTimer() (100-1000ms intervals)
   └─ Checks: if satellites >= 3
      ├─ YES: account->Commit() + Publish()  [Data is valid, update subscribers]
      │       └─ Plays "Connect" sound via MusicPlayer
      └─ NO: Silence or play "Disconnect" sound

3. Recorder & TrackFilter
   ↓
   DP_Recorder subscribes to GPS updates
   └─ Writes points to SD card with timestamp
   
   DP_TrackFilter subscribes to GPS updates
   └─ Applies Douglas-Peucker algorithm
   └─ Removes GPS noise/jitter

4. SportStatus
   ↓
   Processes GPS track
   └─ Calculates: distance, speed, altitude, steps
   └─ Updates statistics

5. UI Layer (LVGL)
   ↓
   LiveMap page subscribes to GPS updates
   ├─ Updates map view
   ├─ Draws heading arrow
   └─ Updates location marker

   StatusBar page subscribes to GPS updates
   └─ Updates GPS signal indicator (📡 icon)
```

---

## 4. Application Initialization Sequence

```cpp
void App_Init() {
    // 1. Initialize Page Manager & Factory
    static AppFactory factory;
    static PageManager manager(&factory);
    
    // 2. Setup LVGL input group for encoder/buttons
    if(!lv_group_get_default()) {
        lv_group_t* group = lv_group_create();
        lv_group_set_default(group);
    }
    
    // 3. Initialize Data Processors (pub/sub system)
    DataProc_Init();
    ACCOUNT_SEND_CMD(Storage, STORAGE_CMD_LOAD);      // Load saved data from SD
    ACCOUNT_SEND_CMD(SysConfig, SYSCONFIG_CMD_LOAD);  // Load settings
    
    // 4. Configure Screen Appearance
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    
    // 5. Initialize Resource Pool (fonts, images)
    ResourcePool::Init();
    
    // 6. Create Status Bar
    Page::StatusBar_Create(lv_layer_top());
    
    // 7. Register & Install Pages
    manager.Install("LiveMap",     "Pages/LiveMap");
    manager.Install("Dialplate",   "Pages/Dialplate");
    manager.Install("SystemInfos", "Pages/SystemInfos");
    manager.Install("Startup",     "Pages/Startup");
    
    // 8. Show Startup Page
    manager.Push("Pages/Startup");
}

void App_Uninit() {
    // Called on power-off
    ACCOUNT_SEND_CMD(SysConfig, SYSCONFIG_CMD_SAVE);
    ACCOUNT_SEND_CMD(Storage,   STORAGE_CMD_SAVE);
    ACCOUNT_SEND_CMD(Recorder,  RECORDER_CMD_STOP);
}
```

---

## 5. Core Data Structures

### 5.1 GPS Information
```cpp
HAL::GPS_Info_t {
    float latitude;          // Current latitude (degrees)
    float longitude;         // Current longitude (degrees)
    float altitude;          // Altitude above sea level (meters)
    float speed;             // Speed (kph)
    float heading;           // Direction of travel (0-360°)
    uint8_t satellites;      // Number of tracked satellites
    uint8_t fix_quality;     // Fix quality level
    uint64_t timestamp;      // UTC timestamp in milliseconds
}
```

### 5.2 IMU Information
```cpp
HAL::IMU_Info_t {
    int16_t accel_x;        // Acceleration X (raw units)
    int16_t accel_y;        // Acceleration Y (raw units)
    int16_t accel_z;        // Acceleration Z (raw units)
    int16_t gyro_x;         // Angular velocity X (raw units)
    int16_t gyro_y;         // Angular velocity Y (raw units)
    int16_t gyro_z;         // Angular velocity Z (raw units)
    int16_t temp;           // Temperature sensor reading
}
```

### 5.3 System Configuration
```cpp
SysConfig_Info_t {
    float longitude;        // Reference longitude for maps
    float latitude;         // Reference latitude for maps
    int16_t timeZone;       // UTC offset in hours (-12 to +14)
    bool soundEnable;       // Audio feedback enable flag
    char language[8];       // Language code (e.g., "en", "zh")
    char arrowTheme[16];    // Heading arrow visual style
    char mapDirPath[16];    // Directory path to map tiles
    char mapExtName[8];     // Map file extension
    bool mapWGS84;          // WGS84 coordinate system flag
}
```

---

## 6. State Management & Pub/Sub Pattern

### DataCenter Architecture
Built on **Account** abstraction:

```cpp
// Publisher (Data Processor)
void DP_GPS_onTimer(Account* account) {
    HAL::GPS_Info_t gpsInfo;
    HAL::GPS_GetInfo(&gpsInfo);
    
    account->Commit(&gpsInfo, sizeof(gpsInfo));  // Update internal state
    account->Publish();                           // Notify all subscribers
}

// Subscriber (Page/UI Component)
class LiveMapPage {
    void onGPSUpdate(GPS_Info_t* data) {
        updateMapMarker(data->latitude, data->longitude);
        updateHeadingArrow(data->heading);
    }
};
```

### Key Features:
- **Lock-free** - Uses atomic operations, no mutexes
- **Type-safe** - Compile-time message verification
- **Asynchronous** - Non-blocking data publishing
- **Efficient** - PingPongBuffer for zero-copy data transfer

---

## 7. Hardware Support

### Supported Platforms
1. **STM32F403** (MDK-ARM_F403A)
   - ARM Cortex-M4 microcontroller
   - For embedded device hardware

2. **STM32F435** (MDK-ARM_F435)
   - ARM Cortex-M4 microcontroller
   - Alternative variant

3. **Linux/SDL2** (LinuxSDL2/)
   - PC simulation with SDL2 graphics
   - For development and testing

4. **Windows Simulator** (Simulator/LVGL.Simulator/)
   - Visual Studio-based simulation
   - Complete UI development environment

### Sensor Integration
- **GPS/GNSS Module** - Via UART (serial communication)
- **IMU (LSM6DSM)** - 6-axis acceleration + gyroscope via I2C
- **Magnetometer (LIS3MDL)** - Compass via I2C
- **Real-Time Clock** - Battery-backed system time
- **SD Card** - For data logging and storage
- **Rotary Encoder** - User input interface
- **LED/Backlight** - Display control
- **Buzzer/Audio** - Audio feedback

---

## 8. Feature Set

### Core Tracking Features
- **GPS Tracking**: Real-time position, speed, heading
- **Track Recording**: Automatic logging of GPS points
- **Track Filtering**: Noise removal using Douglas-Peucker algorithm
- **Route Replay**: Display recorded tracks on map
- **Statistics**: Distance, speed, altitude, step count

### User Interface
- **Multiple Watch Faces**: Dialplate, LiveMap, SystemInfos
- **Status Bar**: Real-time system status (battery, GPS, time)
- **Rotary Encoder Navigation**: Intuitive input interface
- **Multi-language Support**: Configurable language
- **Map Display**: Real-time GPS mapping

### System Management
- **Power Management**: Battery optimization, low-power modes
- **Configuration Persistence**: Settings saved to SD card
- **Time Zone Support**: Configurable timezone conversion
- **Audio Feedback**: User interaction sound cues
- **System Diagnostics**: Memory, stack, performance monitoring

---

## 9. Key Design Patterns

### MVP (Model-View-Presenter)
- **Model**: DataProc layer (data processing)
- **View**: LVGL pages (UI rendering)
- **Presenter**: PageManager (navigation, state sync)

### Observer Pattern (Pub/Sub)
- DataCenter-based event publishing
- Loose coupling between sensors and UI
- Multiple subscribers per data stream

### Factory Pattern
- AppFactory creates pages dynamically
- PageManager instantiates pages on demand

### Singleton Pattern
- DataCenter::Center() - Single global pub/sub hub
- PageManager - Single navigation stack
- ResourcePool - Single resource container

---

## 10. Configuration & Compilation

### Build Systems
- **Keil MDK-ARM**: STM32 embedded development (`MDK-ARM_F403A/`, `MDK-ARM_F435/`)
- **GCC Makefile**: Linux/Linux-SDL2 builds
- **Visual Studio**: Windows simulator (`Simulator/LVGL.Simulator.sln`)

### Key Configuration Files
- `CONFIG_MONKEY_TEST_ENABLE` - Random fuzzing/stress test mode
- `CONFIG_GPS_REFR_PERIOD` - GPS update interval
- `HAL_Config.h` - Hardware-specific parameters
- `lv_conf.h` - LVGL library configuration

---

## 11. Key Dependencies

### Third-Party Libraries
| Library | Purpose |
|---------|---------|
| **LVGL 8.0+** | Graphics framework |
| **TinyGPS++** | GPS sentence parsing |
| **Adafruit GFX** | Graphics primitives |
| **Adafruit ST7789** | LCD controller driver |
| **LSM6DSM** | IMU sensor library |
| **LIS3MDL** | Magnetometer library |
| **SdFat** | SD card filesystem |
| **cm_backtrace** | Crash backtrace analysis |
| **ArduinoAPI** | Embedded compatibility layer |

---

## 12. Critical Flow Diagrams

### Sensor Data Flow
```
┌──────────────┐
│ Sensor (GPS) │
└──────┬───────┘
       │ HAL_GPS_GetInfo()
       │
       ▼
┌──────────────────┐
│ DP_GPS Processor │──→ Publish GPS_Info_t
└──────┬───────────┘
       │ Subscribers:
       ├─→ DP_Recorder (save to SD)
       ├─→ DP_TrackFilter (noise filter)
       ├─→ DP_SportStatus (calculate stats)
       └─→ LiveMapPage (update UI)
```

### User Input Flow
```
┌─────────────────┐
│ Rotary Encoder  │
└────────┬────────┘
         │ HAL_Encoder_GetEvent()
         │
         ▼
┌─────────────────┐
│ PageManager     │
└────────┬────────┘
         │
         ├─→ Page Navigation
         │
         ▼
┌─────────────────┐
│ Current Page    │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ LVGL Rendering  │
└─────────────────┘
```

---

## Summary

X-TRACK is a sophisticated **multi-platform GPS tracking device** with:
- **Hardware Abstraction**: Portable across STM32, Linux, Windows
- **Reactive Data Flow**: Pub/sub pattern for sensor→UI communication
- **Rich UI**: LVGL-based pages with MVP architecture
- **Data Persistence**: SD card storage with key-value configuration
- **Real-time Processing**: Immediate sensor data handling with optimized power management
- **Modular Design**: Clear separation of concerns (HAL, DataProc, Pages)

The core loop continuously reads hardware, processes data through DataCenter subscribers, and updates the GUI at 60 FPS, all while maintaining low power consumption through intelligent sleep modes.
