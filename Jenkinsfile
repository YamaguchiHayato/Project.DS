pipeline {
    agent any

    environment {
        IMAGE            = 'dx12-buildtools:latest'
        SOLUTION         = 'Game.sln'
        SOLUTION_DIR     = 'GameTemplate\\Game'
        CONFIGURATION    = 'Debug'
        PLATFORM         = 'x64'
        JENKINS_HOME_IN  = 'C:\\Jenkins\\home'
        JENKINS_HOME_OUT = 'C:\\jenkins_home'
        DOCKER           = 'C:\\docker-cli\\docker.exe'
    }

    options {
        timestamps()
        timeout(time: 60, unit: 'MINUTES')
        buildDiscarder(logRotator(numToKeepStr: '20'))
    }

    triggers {
        pollSCM('H/5 * * * *')
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Resolve host path') {
            steps {
                script {
                    env.HOST_WS = env.WORKSPACE.replace(env.JENKINS_HOME_IN, env.JENKINS_HOME_OUT)
                    echo "container ws : ${env.WORKSPACE}"
                    echo "host ws      : ${env.HOST_WS}"
                }
            }
        }

        stage('Restore') {
            steps {
                bat """
                    "%DOCKER%" run --rm -m 4g ^
                      -v "%HOST_WS%:C:\\src" -w C:\\src\\%SOLUTION_DIR% ^
                      %IMAGE% ^
                      "msbuild %SOLUTION% /t:Restore /nologo /v:minimal"
                """
            }
        }

        stage('Build') {
            steps {
                bat """
                    "%DOCKER%" run --rm -m 6g --cpus 8 ^
                      -v "%HOST_WS%:C:\\src" -w C:\\src\\%SOLUTION_DIR% ^
                      %IMAGE% ^
                      "chcp 65001 && msbuild %SOLUTION% /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM% /m /nologo /v:normal /fl /flp:logfile=msbuild.log;verbosity=normal"
                """
            }
        }

        stage('Package') {
            steps {
                bat """
                    if exist dist rmdir /s /q dist
                    mkdir dist
                    xcopy /E /I /Y %SOLUTION_DIR%\\%PLATFORM%\\%CONFIGURATION% dist\\%CONFIGURATION%
                """
                archiveArtifacts artifacts: 'dist/**/*', fingerprint: true
            }
        }
    }

    post {
        always {
            recordIssues(tools: [msBuild(pattern: "${SOLUTION_DIR}/msbuild.log")])
            archiveArtifacts artifacts: "${SOLUTION_DIR}/msbuild.log", allowEmptyArchive: true
        }
        success {
            echo 'BUILD OK'
        }
        failure {
            echo 'BUILD FAILED - コンソール出力を確認してください'
        }
        cleanup {
            cleanWs()
        }
    }
}