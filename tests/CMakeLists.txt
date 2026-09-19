add_executable(lingnest_single_instance_tests
    app/SingleInstanceGuardTest.cpp
)

target_link_libraries(lingnest_single_instance_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_single_instance_tests)

add_test(
    NAME lingnest_single_instance
    COMMAND lingnest_single_instance_tests
)

add_executable(lingnest_memory_repository_tests
    memory/MemoryRepositoryTest.cpp
)

target_link_libraries(lingnest_memory_repository_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_memory_repository_tests)

add_test(
    NAME lingnest_memory_repository
    COMMAND lingnest_memory_repository_tests
)

add_executable(lingnest_prompt_builder_tests
    memory/PromptBuilderTest.cpp
)

target_link_libraries(lingnest_prompt_builder_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_prompt_builder_tests)

add_test(
    NAME lingnest_prompt_builder
    COMMAND lingnest_prompt_builder_tests
)

add_executable(lingnest_character_loader_tests
    character/CharacterLoaderTest.cpp
)

target_link_libraries(lingnest_character_loader_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

target_compile_definitions(lingnest_character_loader_tests PRIVATE
    LINGNEST_TEST_CHARACTER_DIR="${CMAKE_SOURCE_DIR}/characters/yuai"
)

lingnest_enable_warnings(lingnest_character_loader_tests)

add_test(
    NAME lingnest_character_loader
    COMMAND lingnest_character_loader_tests
)

add_executable(lingnest_config_manager_tests
    config/ConfigManagerTest.cpp
)

target_link_libraries(lingnest_config_manager_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_config_manager_tests)

add_test(
    NAME lingnest_config_manager
    COMMAND lingnest_config_manager_tests
)

add_executable(lingnest_pet_window_tests
    ${CMAKE_SOURCE_DIR}/resources.qrc
    ui/PetWindowControllerTest.cpp
)

target_link_libraries(lingnest_pet_window_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
    Qt${QT_VERSION_MAJOR}::Test
)

target_compile_definitions(lingnest_pet_window_tests PRIVATE
    LINGNEST_TEST_CHARACTER_DIR="${CMAKE_SOURCE_DIR}/characters/yuai"
)

lingnest_enable_warnings(lingnest_pet_window_tests)

add_test(
    NAME lingnest_pet_window
    COMMAND lingnest_pet_window_tests
)

set_tests_properties(lingnest_pet_window PROPERTIES
    ENVIRONMENT "QT_QUICK_BACKEND=software;LINGNEST_DISABLE_SYSTEM_MOVE=1"
)

add_executable(lingnest_animation_tests
    animation/AnimationPlayerTest.cpp
)

target_link_libraries(lingnest_animation_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_animation_tests)

add_test(
    NAME lingnest_animation
    COMMAND lingnest_animation_tests
)

add_executable(lingnest_pet_state_machine_tests
    pet/PetStateMachineTest.cpp
)

target_link_libraries(lingnest_pet_state_machine_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_pet_state_machine_tests)

add_test(
    NAME lingnest_pet_state_machine
    COMMAND lingnest_pet_state_machine_tests
)

add_executable(lingnest_pet_scheduler_tests
    pet/PetSchedulerTest.cpp
)

target_link_libraries(lingnest_pet_scheduler_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_pet_scheduler_tests)

add_test(
    NAME lingnest_pet_scheduler
    COMMAND lingnest_pet_scheduler_tests
)

add_executable(lingnest_interaction_tests
    interaction/InteractionControllerTest.cpp
)

target_link_libraries(lingnest_interaction_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_interaction_tests)

add_test(
    NAME lingnest_interaction
    COMMAND lingnest_interaction_tests
)

add_executable(lingnest_chat_controller_tests
    ui/ChatControllerTest.cpp
)

target_link_libraries(lingnest_chat_controller_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_chat_controller_tests)

add_test(
    NAME lingnest_chat_controller
    COMMAND lingnest_chat_controller_tests
)

add_executable(lingnest_openai_provider_tests
    ai/OpenAICompatibleProviderTest.cpp
)

target_link_libraries(lingnest_openai_provider_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_openai_provider_tests)

add_test(
    NAME lingnest_openai_provider
    COMMAND lingnest_openai_provider_tests
)

add_executable(lingnest_ai_settings_tests
    ui/AISettingsControllerTest.cpp
)

target_link_libraries(lingnest_ai_settings_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
)

lingnest_enable_warnings(lingnest_ai_settings_tests)

add_test(
    NAME lingnest_ai_settings
    COMMAND lingnest_ai_settings_tests
)

add_executable(lingnest_system_tray_tests
    ui/SystemTrayControllerTest.cpp
)

target_link_libraries(lingnest_system_tray_tests PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
)

lingnest_enable_warnings(lingnest_system_tray_tests)

add_test(
    NAME lingnest_system_tray
    COMMAND lingnest_system_tray_tests
)
