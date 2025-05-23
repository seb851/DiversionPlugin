# UnrealDiversion
Unreal Plugin for Diversion SCM


Running custom build of the generator:

Build the AgentAPI:
`java -jar .\openapi_templates\openapi-generator-cli-7.7.0-SNAPSHOT.jar generate -i .\Source\AgentAPI\agentapi.yaml -g cpp-ue4 -c .\Source\AgentAPI\config.yaml  -t .\openapi_templates\ -o .\Source\AgentAPI\`

Build the CoreAPI:
`java -jar .\openapi_templates\openapi-generator-cli-7.7.0-SNAPSHOT.jar generate -i .\Source\CoreAPI\coreapi.yml -g cpp-ue4 -c .\Source\CoreAPI\config.yaml  -t .\openapi_templates\ -o .\Source\CoreAPI\`