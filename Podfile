# Uncomment the next line to define a global platform for your project
# platform :ios, '13.0'

target 'CommonDevArchiProject' do
  # Comment the next line if you don't want to use dynamic frameworks
  use_frameworks!
	pod 'YYKit'
	pod 'Colours'
	pod 'SDWebImage', '~> 5.0'
	pod 'SVGKit', :git => 'https://github.com/SVGKit/SVGKit.git', :branch => '3.x'
	pod 'WCDB.objc'
	pod 'AFNetworking', '~> 3.1.0'
	pod 'ViaBus'
	pod 'FCAlertView'
	pod "Aspects"

  # Pods for CommonDevArchiProject

end


post_install do |installer|
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      config.build_settings["IPHONEOS_DEPLOYMENT_TARGET"] = "11.0"
    end
  end
end