#r "SendGrid"
using System;
using SendGrid.Helpers.Mail;

public static Mail Run(TraceWriter log, string eventHubMessage)
{
    Mail message = new Mail{};
    Content content = new Content
    {
        Type = "text/plain",
        Value = eventHubMessage
    };
    message.AddContent(content);
    return message;
}