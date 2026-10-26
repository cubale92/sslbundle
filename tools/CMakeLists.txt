# Dev tools: one snapshot/test executable per plug-in (each plug-in defines createPluginFilter()).
foreach(plug Channel4K Bus4K Listen4K)
    set(tool ${plug}_snapshot)
    juce_add_console_app(${tool} PRODUCT_NAME "${tool}")
    target_sources(${tool} PRIVATE
        snapshot.cpp
        ${CMAKE_SOURCE_DIR}/plugins/${plug}/PluginProcessor.cpp
        ${CMAKE_SOURCE_DIR}/plugins/${plug}/PluginEditor.cpp)
    target_include_directories(${tool} PRIVATE ${CMAKE_SOURCE_DIR}/shared ${CMAKE_SOURCE_DIR}/plugins/${plug})
    target_compile_definitions(${tool} PRIVATE
        JucePlugin_Name="8cee ${plug}"
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(${tool} PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endforeach()
