import vscode

def say_hello():
    vscode.window.showInformationMessage('Hello, world!')

def activate(context):
    context.subscriptions.append(vscode.commands.registerCommand('extension.sayHello', say_hello))
