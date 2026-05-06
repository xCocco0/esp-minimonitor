# Minimonitor components

#### minimonitor

Main Minimonitor components. It contains the initialization logic, shared code and the event loop task; other
components depend on this one.

#### mm_utils

Contains mostly utilities and code that is shared among components but does not belong to Minimonitor logic specifically.

#### mm_port

Contains board-specific code.

#### mm_ui

Handles the LVGL UI. This is the only component that depends on LVGL.

#### mm_cli

Contains code to create the CLI interface and CLI commands definitions.

#### mm_io

Contains code relative to other IO devices that don't fall in other categories.

#### mm_net

Handles the network stack and related applications.

## Dependencies

`A --► B` : `A` depends on `B`

```
┌───────────────┐                                                      
│               │                                                      
│    mm_utils   │                                                      
│               │                                                      
└───────▲───────┘                                                                                  
        │                                                              
        U                                                              
                                                                       
                                                                       
        U                          U                                   
        ▲                          ▲                                   
┌───────┴───────┐          ┌───────┴───────┐                           
│               │          │               │                           
│    mm_port    │          │  minimonitor  │                           
│               │          │               │                           
└───────▲───────┘          └───────▲───────┘                           
        │                          │                                   
        ├─────────────────┬────────┴────────┬─────────────────┐        
      U │               U │               U │               U │        
      ▲ │               ▲ │               ▲ │               ▲ │        
┌─────┴─┴───────┐ ┌─────┴─┴───────┐ ┌─────┴─┴───────┐ ┌─────┴─┴───────┐
│               │ │               │ │               │ │               │
│     mm_ui     │ │     mm_io     │ │     mm_net    │ │     mm_cli    │
│               │ │               │ │               │ │               │
└───────────────┘ └───────────────┘ └───────────────┘ └───────────────┘
```