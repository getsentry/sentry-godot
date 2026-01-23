const path = require('path');
const TerserPlugin = require('terser-webpack-plugin');

module.exports = {
	mode : 'production',
	entry : './src/sentry-bridge.ts',
	output : {
		filename : 'sentry-bundle.js',
		path : path.resolve(__dirname, 'dist'),
		library : {
			name : 'SentryBridge',
			type : 'umd',
			export : 'default'
		},
		globalObject : 'this',
		clean : true
	},
	resolve : {
		extensions : [ '.ts', '.js' ]
	},
	module : {
		rules : [
			{
				test : /\.ts$/,
				use : 'ts-loader',
				exclude : /node_modules/
			}
		]
	},
	optimization : {
		minimizer : [
			new TerserPlugin({
				terserOptions : {
					format : {
						comments : false
					},
					compress : {
						drop_console : false,
						drop_debugger : true,
						dead_code : true
					}
				},
				extractComments : false
			})
		]
	},
	devtool : 'source-map'
};
