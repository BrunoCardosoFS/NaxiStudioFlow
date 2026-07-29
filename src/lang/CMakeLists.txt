set(TS_FILES
    ts/pt_BR.ts
    ts/en_US.ts
)

qt_add_translations(${EXECUTABLE_NAME}
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

foreach(qm_file ${QM_FILES})
    get_filename_component(qm_name ${qm_file} NAME)

    set_source_files_properties(${qm_file} PROPERTIES
        QT_RESOURCE_ALIAS ${qm_name}
    )
endforeach()

qt_add_resources(${EXECUTABLE_NAME} "translations"
    PREFIX "/lang"
    FILES ${QM_FILES}
)