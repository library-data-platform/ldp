@Library ('folio_jenkins_shared_libs') _
pipeline {

  environment {
    VERSION = '1.1'
    name = 'ldp'
  }

  options {
    timeout(30)
    buildDiscarder(logRotator(numToKeepStr: '30'))
  }

  agent {
    node {
      label 'jenkins-agent-java11'
    }
  }

  stages {
    stage ('Setup') {
      steps {
        dir(env.WORKSPACE) {
          script {
            def foliociLib = new org.folio.foliociCommands()

            // if release 
            if ( foliociLib.isRelease() ) {
              env.isRelease = true 
              env.dockerRepo = 'folioorg'
              env.version = env.VERSION
            }
            else {
              env.dockerRepo = 'folioci'
              env.version = "${env.VERSION}-master.${env.BUILD_NUMBER}"
            }
          }
        }
        sendNotifications 'STARTED'  
      }
    }
    
    stage('Build Docker') {
      steps {
        script {
          buildDocker {
            publishMaster = 'yes'
            healthChk = 'no'
          }
        }
      } 
    }

    stage('Publish Module Descriptor') {
      when {
        anyOf { 
          branch 'master'
          expression { return env.isRelease }
        }
      }
      steps {
        script {
          def foliociLib = new org.folio.foliociCommands()
          foliociLib.updateModDescriptor(env.modDescriptor) 
        }
        postModuleDescriptor(env.modDescriptor)
      }
    }

  } // end stages

  post {
    always {
      dockerCleanup()
      sendNotifications currentBuild.result 
    }
  }
}
