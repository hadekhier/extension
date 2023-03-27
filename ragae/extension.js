// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
const vscode = require('vscode');

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	console.log('Congratulations, your extension "ragae" is now active!');

	// The command has been defined in the package.json file
	// Now provide the implementation of the command with  registerCommand
	// The commandId parameter must match the command field in package.json
	let panel1 = vscode.window.createWebviewPanel(
		'ragae', // Identifies the panel
		'Ragae Input', // Title of the panel displayed to the user
		vscode.ViewColumn.One, // Panel is shown in the first column of the editor area
		{
			// Enable scripts in the webview
			enableScripts: true,
		}
	);
	const fs = require('fs');
	const path = require('path');
	const uiFilePath = path.join(context.extensionPath, 'ui.html');
	panel1.webview.html = fs.readFileSync(uiFilePath, 'utf8');
	panel1.webview.onDidReceiveMessage((message) => {
		// Write the inputs submitted to a file named input.txt
		const inputFilePath = path.join(context.extensionPath, 'input.txt');
		fs.writeFileSync(inputFilePath, message.inputs);

		// Close the panel
		panel1.dispose();
	});


	let disposable = vscode.commands.registerCommand('ragae.helloWorld', function () {
		// The code you place here will be executed every time your command is executed

		// Display a message box to the user
		vscode.window.showInformationMessage('Hello World from ragae!');
	});

	context.subscriptions.push(disposable);
}

// This method is called when your extension is deactivated
function deactivate() {}

module.exports = {
	activate,
	deactivate
}
