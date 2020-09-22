@Library ('folio_jenkins_shared_libs') _
pipeline {

  environment {
    bare_version = '1.1'
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
              env.version = env.bare_version
            }
            else {
              env.dockerRepo = 'folioci'
              env.version = "${env.bare_version}-${env.BRANCH_NAME}.${env.BUILD_NUMBER}"
            }
          }
        }
        sendNotifications 'STARTED'  
      }
    }

    stage('Build') { 
      steps {
        dir(env.WORKSPACE) {
          sh "./all.sh"
        }
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

  } // end stages

  post {
    always {
      dockerCleanup()
      sendNotifications currentBuild.result 
    }
  }
}
