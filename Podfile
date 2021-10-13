platform :ios, '12.0'

# Avoid warnings when building dependencies.
inhibit_all_warnings!

workspace 'Cardboard'

project 'sdk/sdk.xcodeproj'

target 'sdk' do
  pod 'Protobuf-C++', '3.18.0'
end

post_install do |installer|
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      if target.name == "Protobuf-C++"
        # Bump iOS target version to match Cardboard SDK's one (see
        # https://github.com/CocoaPods/Specs/blob/fbeb2ed05637cbd85ce9ffb4700fae03dfea5f7e/Specs/1/4/6/Protobuf-C%2B%2B/3.18.0/Protobuf-C%2B%2B.podspec.json#L32).
        config.build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '12.0'

        # Override preprocessor definitions to prevent a redefinition warning
        # caused by the "inherited" variable being added twice (see
        # https://github.com/CocoaPods/Specs/blob/fbeb2ed05637cbd85ce9ffb4700fae03dfea5f7e/Specs/1/4/6/Protobuf-C%2B%2B/3.18.0/Protobuf-C%2B%2B.podspec.json#L40).
        if config.name == "Debug"
          config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = ['POD_CONFIGURATION_DEBUG=1', 'DEBUG=1', 'COCOAPODS=1', 'HAVE_PTHREAD=1']
        else
          config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = ['POD_CONFIGURATION_RELEASE=1', 'COCOAPODS=1', 'HAVE_PTHREAD=1']
        end
      end
    end
  end
end
